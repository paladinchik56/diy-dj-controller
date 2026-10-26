/*
 * USB-MIDI device on top of TinyUSB.
 *
 * Peripheral: USB FS device (PA11 = D-, PA12 = D+), clocked at 48 MHz from
 * PLL / 1.5 (SystemClock_Config). The interrupts are handled here and passed
 * to TinyUSB; the stack itself runs in usb_task (app_tasks.c).
 */

#include "FreeRTOS.h"
#include "task.h"

#include "tusb.h"

#include "main.h"
#include "usb_midi.h"

#define USB_RHPORT              0

/*
 * NVIC priority of the USB interrupts. They call FreeRTOS "FromISR" queue
 * functions, which is only allowed at priorities numerically >= 5
 * (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY in FreeRTOSConfig.h).
 */
#define USB_IRQ_PRIORITY        6U

/* How long D+ is held low so the host notices a disconnect */
#define USB_REENUM_PULSE_MS     10U

/* USB-MIDI cable number: one virtual MIDI port */
#define MIDI_CABLE              0U

/* MIDI status bytes, high nibble; the low nibble is the channel */
#define MIDI_NOTE_OFF           0x80U
#define MIDI_NOTE_ON            0x90U
#define MIDI_CONTROL_CHANGE     0xB0U

/* Set in tud_mount_cb() (USB task), taken by the controls task */
static volatile bool connect_event;

/*
 * The Blue Pill has a fixed pull-up resistor on D+, so the host never sees
 * the device leave when it is reflashed or reset while plugged in. Driving
 * D+ low for a moment looks like an unplug, and the host enumerates again.
 */
static void usb_force_reenumeration(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* GPIO PA12 (USB D+): push-pull output low, then back to the USB block */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    gpio.Pin   = GPIO_PIN_12;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    vTaskDelay(pdMS_TO_TICKS(USB_REENUM_PULSE_MS));

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
}

void usb_midi_init(void)
{
    usb_force_reenumeration();

    /* USB peripheral clock (the 48 MHz USB clock source is set in SystemClock_Config) */
    __HAL_RCC_USB_CLK_ENABLE();

    /* NVIC: USB high- and low-priority interrupts; TinyUSB enables them itself */
    HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, USB_IRQ_PRIORITY, 0);
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, USB_IRQ_PRIORITY, 0);

    const tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    if (!tusb_init(USB_RHPORT, &dev_init)) {
        Error_Handler();
    }
}

void usb_midi_task(void)
{
    tud_task();
}

bool usb_midi_is_mounted(void)
{
    return tud_midi_mounted();
}

bool usb_midi_take_connect_event(void)
{
    bool event;

    taskENTER_CRITICAL();
    event = connect_event;
    connect_event = false;
    taskEXIT_CRITICAL();

    return event;
}

static bool midi_send3(uint8_t status, uint8_t data1, uint8_t data2)
{
    if (!tud_midi_mounted()) {
        return false;
    }

    const uint8_t msg[3] = { status, (uint8_t)(data1 & 0x7FU), (uint8_t)(data2 & 0x7FU) };
    return tud_midi_stream_write(MIDI_CABLE, msg, sizeof msg) == sizeof msg;
}

bool usb_midi_send_note(uint8_t channel, uint8_t note, bool on, uint8_t velocity)
{
    const uint8_t status = (uint8_t)((on ? MIDI_NOTE_ON : MIDI_NOTE_OFF) | (channel & 0x0FU));
    return midi_send3(status, note, on ? velocity : 0U);
}

bool usb_midi_send_cc(uint8_t channel, uint8_t controller, uint8_t value)
{
    return midi_send3((uint8_t)(MIDI_CONTROL_CHANGE | (channel & 0x0FU)), controller, value);
}

bool usb_midi_send_cc14(uint8_t channel, uint8_t controller, uint16_t value)
{
    /* MSB first: receivers apply the LSB to the most recent MSB */
    const bool msb_ok = usb_midi_send_cc(channel, controller, (uint8_t)(value >> 7));
    const bool lsb_ok = usb_midi_send_cc(channel, (uint8_t)(controller + 32U), (uint8_t)(value & 0x7FU));
    return msb_ok && lsb_ok;
}

/* ---- TinyUSB callbacks (run in the USB task) ---- */

void tud_mount_cb(void)
{
    connect_event = true;
}

/*
 * Data from the host (e.g. LED feedback from DJ software) is not used yet.
 * Read it out anyway: an unread RX FIFO stops the host from sending more.
 */
void tud_midi_rx_cb(uint8_t itf)
{
    uint8_t packet[4];
    while (tud_midi_n_packet_read(itf, packet)) {
    }
}

/* ---- Interrupt handlers (names from the vector table in startup_stm32f103xb.s) ---- */

void USB_HP_CAN1_TX_IRQHandler(void)
{
    tusb_int_handler(USB_RHPORT, true);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    tusb_int_handler(USB_RHPORT, true);
}
