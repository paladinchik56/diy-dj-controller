#ifndef DJ_MIDI_MAP_H
#define DJ_MIDI_MAP_H

#include "buttons.h"

/*
 * Maps panel controls to MIDI messages and sends them over USB-MIDI.
 * The full table is documented in MIDI_MAP.md (for MIDI Learn in DJ software).
 */

void midi_map_init(void);

/*
 * Queues Note On/Off for a debounced button event (Shift layer included).
 * Dropped while USB is not connected.
 */
void midi_map_on_button(const button_event_t *event);

/*
 * Sends the queued notes, in order, as far as the USB buffer has room; the
 * rest stays queued for the next call. Call after every buttons_scan().
 */
void midi_map_send_notes(void);

/*
 * Sends every analog control whose current output differs from the value
 * last sent. Call after each analog_process(). Messages that did not fit into
 * the USB buffer are simply retried on the next call, and after each new
 * connection to the host all controls are sent once, so the DJ software
 * always knows the real fader and knob positions.
 */
void midi_map_sync_analog(void);

#endif //DJ_MIDI_MAP_H
