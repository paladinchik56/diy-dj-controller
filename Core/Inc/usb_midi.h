#ifndef DJ_USB_MIDI_H
#define DJ_USB_MIDI_H

#include <stdbool.h>
#include <stdint.h>

/*
 * USB-MIDI device on top of TinyUSB (lib/tinyusb).
 * Peripheral: USB FS device, PA11 = D-, PA12 = D+.
 *
 * MIDI channels here are 0..15 (shown as 1..16 in DJ software),
 * notes, controller numbers and 7-bit values are 0..127.
 */

/*
 * Forces a host re-enumeration, then starts the TinyUSB device stack.
 * Call from the USB task after the scheduler is running: the USB interrupt
 * uses FreeRTOS queue calls.
 */
void usb_midi_init(void);

/* Runs the TinyUSB device stack; blocks until there is a USB event. Call in a loop. */
void usb_midi_task(void);

/* true while the host has configured the device and MIDI can be sent */
bool usb_midi_is_mounted(void);

/*
 * true once after each new connection to the host (and after power-up while
 * connected); use it to send the full panel state so the software syncs.
 */
bool usb_midi_take_connect_event(void);

/* Note On with the given velocity (1..127), or Note Off. false if not sent. */
bool usb_midi_send_note(uint8_t channel, uint8_t note, bool on, uint8_t velocity);

/* Control Change, 7-bit value. false if not sent. */
bool usb_midi_send_cc(uint8_t channel, uint8_t controller, uint8_t value);

/*
 * 14-bit Control Change: MSB on `controller` (0..31) and LSB on
 * `controller + 32`, the standard MIDI pairing. value 0..16383.
 * false if not sent.
 */
bool usb_midi_send_cc14(uint8_t channel, uint8_t controller, uint16_t value);

#endif //DJ_USB_MIDI_H
