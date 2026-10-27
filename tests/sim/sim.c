/*
 * Host simulation: fake HAL / FreeRTOS / TinyUSB, simulated panel and USB
 * host, and the controls_task loop. See sim.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "tusb.h"

#include "adc.h"
#include "analog.h"
#include "buttons.h"
#include "main.h"
#include "midi_map.h"
#include "usb_midi.h"

#include "sim.h"

/* ------------------------------------------------------------------------
 * Expected wiring and MIDI map, written from PINOUT.md and MIDI_MAP.md
 * ------------------------------------------------------------------------ */

const sim_button_t sim_buttons[] = {
    { "PLAY_A", 'A', 2,  0, 0 },
    { "CUE_A",  'A', 3,  0, 1 },
    { "SYNC_A", 'A', 4,  0, 2 },
    { "SHIFT",  'A', 5,  2, 0 },
    { "PLAY_B", 'A', 6,  1, 0 },
    { "CUE_B",  'A', 7,  1, 1 },
    { "SYNC_B", 'B', 3,  1, 2 },
    { "HC1_A",  'B', 4,  0, 3 },
    { "HC2_A",  'B', 5,  0, 4 },
    { "HC3_A",  'B', 6,  0, 5 },
    { "HC4_A",  'B', 7,  0, 6 },
    { "HC1_B",  'B', 8,  1, 3 },
    { "HC2_B",  'B', 9,  1, 4 },
    { "HC3_B",  'B', 10, 1, 5 },
    { "HC4_B",  'B', 11, 1, 6 },
};
const size_t sim_button_count = sizeof sim_buttons / sizeof sim_buttons[0];

const sim_analog_t sim_analogs[] = {
    { "MASTER",     'A', 0,  2, 20, false, POT_MASTER },
    { "GAIN_A",     'A', 1,  0, 20, false, POT_GAIN_A },
    { "BASS_A",     'A', 2,  0, 21, false, POT_BASS_A },
    { "MID_A",      'A', 3,  0, 22, false, POT_MID_A },
    { "TREBLE_A",   'A', 4,  0, 23, false, POT_TREBLE_A },
    { "FX1_A",      'A', 5,  0, 24, false, POT_FX1_A },
    { "FX2_A",      'A', 6,  0, 25, false, POT_FX2_A },
    { "GAIN_B",     'A', 7,  1, 20, false, POT_GAIN_B },
    { "BASS_B",     'A', 8,  1, 21, false, POT_BASS_B },
    { "MID_B",      'A', 9,  1, 22, false, POT_MID_B },
    { "TREBLE_B",   'A', 10, 1, 23, false, POT_TREBLE_B },
    { "FX1_B",      'A', 11, 1, 24, false, POT_FX1_B },
    { "FX2_B",      'A', 12, 1, 25, false, POT_FX2_B },
    { "PITCH_A",    'B', 0,  0, 16, true,  FADER_PITCH_A },
    { "PITCH_B",    'B', 1,  1, 16, true,  FADER_PITCH_B },
    { "CHANNEL_A",  'B', 2,  0, 26, false, FADER_CHANNEL_A },
    { "CHANNEL_B",  'B', 3,  1, 26, false, FADER_CHANNEL_B },
    { "CROSSFADER", 'B', 4,  2, 21, false, FADER_CROSSFADER },
};
const size_t sim_analog_count = sizeof sim_analogs / sizeof sim_analogs[0];

const sim_button_t *sim_find_button(const char *name)
{
    for (size_t i = 0; i < sim_button_count; i++) {
        if (strcmp(sim_buttons[i].name, name) == 0) {
            return &sim_buttons[i];
        }
    }
    fprintf(stderr, "sim: unknown button %s\n", name);
    exit(2);
}

const sim_analog_t *sim_find_analog(const char *name)
{
    for (size_t i = 0; i < sim_analog_count; i++) {
        if (strcmp(sim_analogs[i].name, name) == 0) {
            return &sim_analogs[i];
        }
    }
    fprintf(stderr, "sim: unknown analog control %s\n", name);
    exit(2);
}

/* ------------------------------------------------------------------------
 * Simulated panel
 * ------------------------------------------------------------------------ */

#define RAW_MAX         4095

static uint32_t now_ms;
static uint32_t rng_state;
static int noise_amp;

typedef struct {
    bool     target;        /* pressed after the bounce */
    uint32_t bounce_until;
    bool     contact;       /* current electrical state, true = closed */
} sim_button_state_t;

typedef struct {
    double   pos;           /* raw position 0..4095 */
    double   step;          /* per-ms change while ramping */
    uint32_t ramp_until;
    uint16_t ramp_target;
} sim_analog_state_t;

static sim_button_state_t button_state[sizeof sim_buttons / sizeof sim_buttons[0]];
static sim_analog_state_t analog_state[sizeof sim_analogs / sizeof sim_analogs[0]];

static uint32_t rng_next(void)
{
    rng_state = rng_state * 1103515245U + 12345U;
    return rng_state >> 16;
}

void sim_button(const char *name, bool pressed, uint32_t bounce_ms)
{
    const size_t i = (size_t)(sim_find_button(name) - sim_buttons);
    button_state[i].target = pressed;
    button_state[i].bounce_until = now_ms + bounce_ms;
}

void sim_analog(const char *name, uint16_t raw)
{
    const size_t i = (size_t)(sim_find_analog(name) - sim_analogs);
    analog_state[i].pos = raw;
    analog_state[i].ramp_until = 0;
}

void sim_analog_ramp(const char *name, uint16_t raw, uint32_t duration_ms)
{
    const size_t i = (size_t)(sim_find_analog(name) - sim_analogs);
    if (duration_ms == 0) {
        sim_analog(name, raw);
        return;
    }
    analog_state[i].step = ((double)raw - analog_state[i].pos) / duration_ms;
    analog_state[i].ramp_until = now_ms + duration_ms;
    analog_state[i].ramp_target = raw;
}

void sim_set_noise(int amp)
{
    noise_amp = amp;
}

/* Called once per ms before the firmware runs */
static void panel_update(void)
{
    for (size_t i = 0; i < sim_button_count; i++) {
        sim_button_state_t *b = &button_state[i];
        b->contact = (now_ms < b->bounce_until) ? (rng_next() & 1U) != 0 : b->target;
    }
    for (size_t i = 0; i < sim_analog_count; i++) {
        sim_analog_state_t *a = &analog_state[i];
        if (a->ramp_until == 0) {
            continue;
        }
        if (now_ms >= a->ramp_until) {
            a->pos = a->ramp_target;
            a->ramp_until = 0;
        } else {
            a->pos += a->step;
        }
    }
}

/* ------------------------------------------------------------------------
 * Fake HAL
 * ------------------------------------------------------------------------ */

GPIO_TypeDef sim_gpioa = { 'A' }, sim_gpiob = { 'B' }, sim_gpioc = { 'C' };
CoreDebug_Type sim_coredebug;
DWT_Type sim_dwt;
uint32_t SystemCoreClock = 0;
ADC_HandleTypeDef hadc1;

static uint16_t odr_b;          /* output latch of port B (mux address PB12..15) */
static uint32_t adc_channel;
static uint16_t adc_result;

static int pin_number(uint16_t pin_mask)
{
    for (int n = 0; n < 16; n++) {
        if (pin_mask == (uint16_t)(1U << n)) {
            return n;
        }
    }
    fprintf(stderr, "sim: GPIO call with pin mask 0x%04x (not one pin)\n", pin_mask);
    exit(2);
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    const int n = pin_number(pin);
    for (size_t i = 0; i < sim_button_count; i++) {
        if (sim_buttons[i].port == port->name && sim_buttons[i].pin == n) {
            /* Button to GND + internal pull-up: closed contact reads low */
            return button_state[i].contact ? GPIO_PIN_RESET : GPIO_PIN_SET;
        }
    }
    fprintf(stderr, "sim: read of P%c%d, no button there\n", port->name, n);
    exit(2);
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (port == GPIOB) {
        odr_b = state == GPIO_PIN_SET ? (uint16_t)(odr_b | pin) : (uint16_t)(odr_b & ~pin);
    }
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port;
    (void)pin;
}

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port;
    (void)init;
}

void HAL_GPIO_DeInit(GPIO_TypeDef *port, uint32_t pin)
{
    (void)port;
    (void)pin;
}

HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc, ADC_ChannelConfTypeDef *cfg)
{
    (void)hadc;
    adc_channel = cfg->Channel;
    return HAL_OK;
}

/* Conversion: SIG of the mux selected by the ADC channel, at the address on PB12..15 */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    const char mux = adc_channel == ADC_CHANNEL_0 ? 'A' : adc_channel == ADC_CHANNEL_1 ? 'B' : '?';
    const uint8_t address = (uint8_t)((odr_b >> 12) & 0x0FU);

    double pos = -1.0;
    for (size_t i = 0; i < sim_analog_count; i++) {
        if (sim_analogs[i].mux == mux && sim_analogs[i].mux_ch == address) {
            pos = analog_state[i].pos;
        }
    }
    if (pos < 0.0) {
        /* Unwired mux input: floating */
        pos = (double)(rng_next() % (RAW_MAX + 1));
    }

    int raw = (int)(pos + 0.5);
    if (noise_amp > 0) {
        raw += (int)(rng_next() % (uint32_t)(2 * noise_amp + 1)) - noise_amp;
    }
    adc_result = (uint16_t)(raw < 0 ? 0 : raw > RAW_MAX ? RAW_MAX : raw);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout_ms)
{
    (void)hadc;
    (void)timeout_ms;
    return HAL_OK;
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return adc_result;
}

HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t preempt, uint32_t sub)
{
    (void)irq;
    (void)preempt;
    (void)sub;
}

void Error_Handler(void)
{
    fprintf(stderr, "sim: firmware called Error_Handler at t=%u ms\n", now_ms);
    exit(3);
}

void vTaskDelay(TickType_t ticks)
{
    (void)ticks;
}

void debug_printf(const char *fmt, ...)
{
    (void)fmt;
}

/* ------------------------------------------------------------------------
 * Fake TinyUSB: MIDI TX FIFO like midi_device.c in TinyUSB 0.21.0
 * ------------------------------------------------------------------------ */

/* CFG_TUD_MIDI_TX_BUFSIZE (64 bytes) / 4 bytes per USB-MIDI packet */
#define TX_FIFO_PACKETS 16U

typedef struct {
    uint8_t  packet[4];
    uint32_t t_sent_ms;
} fifo_entry_t;

static bool usb_mounted;
static fifo_entry_t tx_fifo[TX_FIFO_PACKETS];
static uint32_t fifo_head, fifo_count;
static uint8_t stream_buf[4], stream_index, stream_total;
static uint32_t fifo_full_events;

bool tusb_init(uint8_t rhport, const tusb_rhport_init_t *init)
{
    (void)rhport;
    (void)init;
    return true;
}

void tusb_int_handler(uint8_t rhport, bool in_isr)
{
    (void)rhport;
    (void)in_isr;
}

void tud_task(void)
{
}

bool tud_midi_mounted(void)
{
    return usb_mounted;
}

bool tud_midi_n_packet_read(uint8_t itf, uint8_t packet[4])
{
    (void)itf;
    (void)packet;
    return false;
}

/* Same per-byte logic as tud_midi_n_stream_write(): a packet needs 4 free
 * bytes before each byte is taken, and goes into the FIFO when complete. */
uint32_t tud_midi_stream_write(uint8_t cable_num, const uint8_t *buffer, uint32_t bufsize)
{
    if (!usb_mounted) {
        return 0;
    }

    uint32_t i = 0;
    while (i < bufsize) {
        if (fifo_count >= TX_FIFO_PACKETS) {
            fifo_full_events++;
            break;
        }
        const uint8_t data = buffer[i++];

        if (stream_index == 0) {
            const uint8_t msg = data >> 4;
            stream_buf[0] = (uint8_t)((cable_num << 4) | msg);
            stream_buf[1] = data;
            stream_index = 2;
            stream_total = ((msg >= 0x8 && msg <= 0xB) || msg == 0xE) ? 4 : 3;
        } else {
            stream_buf[stream_index++] = data;
        }

        if (stream_index == stream_total) {
            fifo_entry_t *e = &tx_fifo[(fifo_head + fifo_count) % TX_FIFO_PACKETS];
            memcpy(e->packet, stream_buf, 4);
            e->t_sent_ms = now_ms;
            fifo_count++;
            stream_index = 0;
        }
    }
    return i;
}

/* ------------------------------------------------------------------------
 * Simulated USB host and "DJ software"
 * ------------------------------------------------------------------------ */

#define LOG_MAX         200000U

static sim_msg_t msg_log[LOG_MAX];
static size_t log_count;
static bool host_notes[16][128];
static uint16_t host_ccs[16][128];
static uint32_t host_packets_per_read = 16;
static uint32_t host_read_interval_ms = 1;
static uint32_t protocol_errors, double_note_on, orphan_note_off;

static void host_receive(const fifo_entry_t *e)
{
    const uint8_t *p = e->packet;
    const uint8_t cin = p[0] & 0x0FU;
    const uint8_t status = p[1];
    const uint8_t ch = status & 0x0FU;
    const uint8_t type = status >> 4;

    if ((p[0] >> 4) != 0 || cin != type || type < 0x8 || type > 0xE ||
        (p[2] & 0x80U) != 0 || (p[3] & 0x80U) != 0) {
        protocol_errors++;
        return;
    }

    if (log_count < LOG_MAX) {
        sim_msg_t *m = &msg_log[log_count++];
        m->t_sent_ms = e->t_sent_ms;
        m->t_rx_ms = now_ms;
        memcpy(m->bytes, &p[1], 3);
    }

    const bool note_on = type == 0x9 && p[3] != 0;
    const bool note_off = type == 0x8 || (type == 0x9 && p[3] == 0);
    if (note_on) {
        double_note_on += host_notes[ch][p[2]];
        host_notes[ch][p[2]] = true;
    } else if (note_off) {
        orphan_note_off += !host_notes[ch][p[2]];
        host_notes[ch][p[2]] = false;
    } else if (type == 0xB) {
        host_ccs[ch][p[2]] = p[3];
    }
}

static void host_frame(void)
{
    if (!usb_mounted || now_ms % host_read_interval_ms != 0) {
        return;
    }
    for (uint32_t n = 0; n < host_packets_per_read && fifo_count > 0; n++) {
        host_receive(&tx_fifo[fifo_head]);
        fifo_head = (fifo_head + 1U) % TX_FIFO_PACKETS;
        fifo_count--;
    }
}

void sim_usb_connect(void)
{
    usb_mounted = true;
    tud_mount_cb();
}

void sim_usb_disconnect(void)
{
    /* TinyUSB closes the endpoint stream: FIFO and partial packet are dropped */
    usb_mounted = false;
    fifo_head = fifo_count = 0;
    stream_index = 0;
    memset(host_notes, 0, sizeof host_notes);
}

void sim_host_rate(uint32_t packets, uint32_t interval_ms)
{
    host_packets_per_read = packets;
    host_read_interval_ms = interval_ms ? interval_ms : 1;
}

size_t sim_log_count(void) { return log_count; }
const sim_msg_t *sim_log_at(size_t i) { return &msg_log[i]; }
uint32_t sim_host_protocol_errors(void) { return protocol_errors; }
uint32_t sim_host_double_note_on(void) { return double_note_on; }
uint32_t sim_host_orphan_note_off(void) { return orphan_note_off; }
uint32_t sim_fifo_full_events(void) { return fifo_full_events; }

size_t sim_messages_since(uint32_t t_ms)
{
    size_t n = 0;
    for (size_t i = 0; i < log_count; i++) {
        n += msg_log[i].t_rx_ms >= t_ms;
    }
    return n;
}

bool sim_host_note_on(uint8_t ch, uint8_t note)
{
    return host_notes[ch][note];
}

uint32_t sim_host_notes_held(void)
{
    uint32_t n = 0;
    for (int ch = 0; ch < 16; ch++) {
        for (int note = 0; note < 128; note++) {
            n += host_notes[ch][note];
        }
    }
    return n;
}

uint16_t sim_host_cc(uint8_t ch, uint8_t cc)
{
    return host_ccs[ch][cc];
}

uint16_t sim_host_value(const sim_analog_t *a)
{
    const uint16_t msb = host_ccs[a->midi_ch][a->cc];
    if (!a->high_res || msb == SIM_UNKNOWN) {
        return msb;
    }
    const uint16_t lsb = host_ccs[a->midi_ch][a->cc + 32];
    return lsb == SIM_UNKNOWN ? SIM_UNKNOWN : (uint16_t)((msb << 7) | lsb);
}

uint16_t sim_fw_output(const sim_analog_t *a)
{
    return analog_output((analog_id_t)a->fw_id);
}

uint16_t sim_ideal_value(const sim_analog_t *a, uint16_t raw)
{
    const int lo = 24, hi = RAW_MAX - 24;   /* END_DEAD_ZONE_RAW */
    const int out_max = a->high_res ? 16383 : 127;
    if (raw <= lo) {
        return 0;
    }
    if (raw >= hi) {
        return (uint16_t)out_max;
    }
    return (uint16_t)(((raw - lo) * out_max + (hi - lo) / 2) / (hi - lo));
}

static const char *describe(const uint8_t *b, char *buf, size_t len)
{
    const uint8_t type = b[0] >> 4, ch = b[0] & 0x0FU;

    if (type == 0x9 || type == 0x8) {
        for (size_t i = 0; i < sim_button_count; i++) {
            const sim_button_t *s = &sim_buttons[i];
            if (s->midi_ch == ch && (b[1] == s->note || (s->note + 64 == b[1] && strcmp(s->name, "SHIFT") != 0))) {
                snprintf(buf, len, "%s%s", s->name, b[1] >= 64 ? " (shift)" : "");
                return buf;
            }
        }
    } else if (type == 0xB) {
        for (size_t i = 0; i < sim_analog_count; i++) {
            const sim_analog_t *s = &sim_analogs[i];
            if (s->midi_ch == ch && b[1] == s->cc) {
                snprintf(buf, len, "%s%s", s->name, s->high_res ? " MSB" : "");
                return buf;
            }
            if (s->high_res && s->midi_ch == ch && b[1] == s->cc + 32) {
                snprintf(buf, len, "%s LSB", s->name);
                return buf;
            }
        }
    }
    snprintf(buf, len, "?");
    return buf;
}

void sim_print_log(size_t from)
{
    char name[32];
    for (size_t i = from; i < log_count; i++) {
        const sim_msg_t *m = &msg_log[i];
        const uint8_t type = m->bytes[0] >> 4;
        const char *kind = type == 0x9 ? (m->bytes[2] ? "Note On " : "Note Off")
                         : type == 0x8 ? "Note Off" : type == 0xB ? "CC      " : "?       ";
        printf("  [%6u ms] %02X %02X %02X  ch%-2u %s %3u = %3u   %s\n",
               m->t_rx_ms, m->bytes[0], m->bytes[1], m->bytes[2],
               (m->bytes[0] & 0x0FU) + 1U, kind, m->bytes[1], m->bytes[2],
               describe(m->bytes, name, sizeof name));
    }
}

/* ------------------------------------------------------------------------
 * Firmware loop: same cadence as controls_task() in app_tasks.c
 * ------------------------------------------------------------------------ */

#define CONTROLS_PERIOD_MS      1U
#define ANALOG_SCAN_PERIOD_MS   4U

static uint32_t period_count;

static void on_button_event(const button_event_t *event)
{
    midi_map_on_button(event);
}

void sim_boot(void)
{
    now_ms = 0;
    rng_state = 12345U;
    noise_amp = 4;
    memset(button_state, 0, sizeof button_state);
    for (size_t i = 0; i < sim_analog_count; i++) {
        analog_state[i] = (sim_analog_state_t){ .pos = RAW_MAX / 2 };
    }

    usb_mounted = false;
    fifo_head = fifo_count = 0;
    stream_index = 0;
    fifo_full_events = 0;
    log_count = 0;
    memset(host_notes, 0, sizeof host_notes);
    memset(host_ccs, 0xFF, sizeof host_ccs);
    host_packets_per_read = 16;
    host_read_interval_ms = 1;
    protocol_errors = double_note_on = orphan_note_off = 0;
    period_count = 0;

    /* usb_task, then controls_task start-up */
    usb_midi_init();
    analog_init();
    buttons_init();
    midi_map_init();
}

void sim_run_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        panel_update();

        buttons_scan(on_button_event);
        if (period_count % (ANALOG_SCAN_PERIOD_MS / CONTROLS_PERIOD_MS) == 0U) {
            analog_scan();
            analog_process(NULL);
            midi_map_sync_analog();
        }
        period_count++;

        /* USB runs at a higher priority than the controls task: the host
         * collects whatever is in the FIFO at the end of this 1 ms frame */
        host_frame();
        now_ms++;
    }
}

uint32_t sim_now(void)
{
    return now_ms;
}
