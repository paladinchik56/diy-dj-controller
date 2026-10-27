#ifndef SIM_TUSB_H
#define SIM_TUSB_H

/*
 * Host simulation stand-in for TinyUSB: the device API used by usb_midi.c.
 * Implemented in sim.c with the same MIDI TX FIFO behaviour as TinyUSB
 * 0.21.0 (midi_device.c), drained by a simulated USB host.
 */

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TUSB_ROLE_DEVICE = 1
} tusb_role_t;

typedef enum {
    TUSB_SPEED_AUTO = 0xFF
} tusb_speed_t;

typedef struct {
    tusb_role_t  role;
    tusb_speed_t speed;
} tusb_rhport_init_t;

bool tusb_init(uint8_t rhport, const tusb_rhport_init_t *init);
void tusb_int_handler(uint8_t rhport, bool in_isr);
void tud_task(void);

bool tud_midi_mounted(void);
uint32_t tud_midi_stream_write(uint8_t cable_num, const uint8_t *buffer, uint32_t bufsize);
bool tud_midi_n_packet_read(uint8_t itf, uint8_t packet[4]);

/* Callbacks implemented by the firmware (usb_midi.c) */
void tud_mount_cb(void);
void tud_midi_rx_cb(uint8_t itf);

#endif //SIM_TUSB_H
