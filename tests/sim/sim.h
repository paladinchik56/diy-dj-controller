#ifndef DJ_SIM_H
#define DJ_SIM_H

/*
 * Host simulation of the whole controller firmware path:
 *   simulated panel (buttons, knobs, faders behind the two muxes)
 *     -> real firmware modules (buttons, debounce, analog, analog_filter,
 *        mux, midi_map, usb_midi), run with the same 1 ms / 4 ms cadence
 *        as controls_task in app_tasks.c
 *     -> TinyUSB MIDI TX FIFO model -> simulated USB host
 *     -> "DJ software" state (notes held, CC values) + a timestamped log.
 *
 * Controls are named as in MIDI_MAP.md / PINOUT.md, e.g. "PLAY_A", "PITCH_B".
 * The expected pins, mux channels, MIDI channels and numbers in sim.c are
 * written from those documents, independently of the firmware tables.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIM_UNKNOWN     0xFFFFU     /* host has not received this value yet */

typedef struct {
    uint32_t t_sent_ms;     /* when the firmware wrote it into the TX FIFO */
    uint32_t t_rx_ms;       /* when the host received it */
    uint8_t  bytes[3];      /* MIDI message: status, data1, data2 */
} sim_msg_t;

typedef struct {
    const char *name;
    char        port;       /* 'A' or 'B' */
    uint8_t     pin;
    uint8_t     midi_ch;    /* 0-based */
    uint8_t     note;
} sim_button_t;

typedef struct {
    const char *name;
    char        mux;        /* 'A' (pots, PA0) or 'B' (faders, PA1) */
    uint8_t     mux_ch;
    uint8_t     midi_ch;    /* 0-based */
    uint8_t     cc;
    bool        high_res;   /* 14-bit: MSB on cc, LSB on cc + 32 */
    int         fw_id;      /* analog_id_t in the firmware */
} sim_analog_t;

extern const sim_button_t sim_buttons[];
extern const size_t sim_button_count;
extern const sim_analog_t sim_analogs[];
extern const size_t sim_analog_count;

const sim_button_t *sim_find_button(const char *name);
const sim_analog_t *sim_find_analog(const char *name);

/* ---- lifetime and time ---- */
void sim_boot(void);                /* fresh panel + firmware init, USB unplugged */
void sim_run_ms(uint32_t ms);       /* advance time in 1 ms steps */
uint32_t sim_now(void);

/* ---- panel inputs ---- */
/* Press or release; the contact bounces randomly for bounce_ms first */
void sim_button(const char *name, bool pressed, uint32_t bounce_ms);
/* Set a knob/fader to a raw ADC position 0..4095 */
void sim_analog(const char *name, uint16_t raw);
/* Move a knob/fader linearly to raw over duration_ms (starts now) */
void sim_analog_ramp(const char *name, uint16_t raw, uint32_t duration_ms);
/* ADC noise amplitude in raw units, +-amp (default 4) */
void sim_set_noise(int amp);

/* ---- USB ---- */
void sim_usb_connect(void);
void sim_usb_disconnect(void);
/* Host reads up to `packets` USB-MIDI packets every `interval_ms`
 * (default 16 per 1 ms = one 64-byte bulk transfer per USB frame) */
void sim_host_rate(uint32_t packets, uint32_t interval_ms);

/* ---- what the host saw ---- */
size_t sim_log_count(void);
const sim_msg_t *sim_log_at(size_t i);
size_t sim_messages_since(uint32_t t_ms);
bool sim_host_note_on(uint8_t ch, uint8_t note);
uint16_t sim_host_cc(uint8_t ch, uint8_t cc);                 /* 0..127 or SIM_UNKNOWN */
uint16_t sim_host_value(const sim_analog_t *a);                /* 7- or 14-bit, or SIM_UNKNOWN */
uint32_t sim_host_protocol_errors(void);  /* malformed packets */
uint32_t sim_host_double_note_on(void);   /* Note On for a note already held */
uint32_t sim_host_orphan_note_off(void);  /* Note Off for a note not held */
uint32_t sim_host_notes_held(void);
uint32_t sim_fifo_full_events(void);      /* firmware writes refused: FIFO full */

/* Firmware's current filtered output for a control (analog_output) */
uint16_t sim_fw_output(const sim_analog_t *a);
/* Ideal value for a raw position: end dead zones + scaling, no noise */
uint16_t sim_ideal_value(const sim_analog_t *a, uint16_t raw);

/* Print the log from entry `from` on, one decoded message per line */
void sim_print_log(size_t from);

#endif //DJ_SIM_H
