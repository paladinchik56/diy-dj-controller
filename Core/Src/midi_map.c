/*
 * Panel -> MIDI mapping. Keep MIDI_MAP.md in sync with the tables below.
 *
 * Generic layout for MIDI Learn:
 *   - deck A on MIDI channel 1, deck B on channel 2, shared controls on 3;
 *     both decks use the same note and CC numbers
 *   - buttons send Note On (velocity 127) / Note Off; with Shift held the note
 *     is MIDI_SHIFT_NOTE_OFFSET higher; Shift itself also sends a note
 *   - knobs and faders send Control Change, the pitch faders a 14-bit CC pair
 *   - CC numbers avoid Bank Select (0/32), Volume (7), the switch range
 *     (64+) and the channel mode messages (120-127)
 */

#include <stdbool.h>
#include <stdint.h>

#include "analog.h"
#include "debug_uart.h"
#include "midi_map.h"
#include "usb_midi.h"

/* Channels are 0-based on the wire: 0 = "channel 1" in DJ software */
#define MIDI_CH_DECK_A          0U
#define MIDI_CH_DECK_B          1U
#define MIDI_CH_SHARED          2U

#define MIDI_NOTE_VELOCITY      127U
#define MIDI_SHIFT_NOTE_OFFSET  64U

/* 1 = print every MIDI message sent (needs the USB-UART adapter) */
#define MIDI_DEBUG_PRINT        0

typedef struct {
    uint8_t channel;
    uint8_t number;     /* note number for buttons, controller number for analog */
} midi_target_t;

static const midi_target_t button_map[BUTTON_COUNT] = {
    [BUTTON_PLAY_A] = { MIDI_CH_DECK_A, 0 },
    [BUTTON_CUE_A]  = { MIDI_CH_DECK_A, 1 },
    [BUTTON_SYNC_A] = { MIDI_CH_DECK_A, 2 },
    [BUTTON_HC1_A]  = { MIDI_CH_DECK_A, 3 },
    [BUTTON_HC2_A]  = { MIDI_CH_DECK_A, 4 },
    [BUTTON_HC3_A]  = { MIDI_CH_DECK_A, 5 },
    [BUTTON_HC4_A]  = { MIDI_CH_DECK_A, 6 },

    [BUTTON_PLAY_B] = { MIDI_CH_DECK_B, 0 },
    [BUTTON_CUE_B]  = { MIDI_CH_DECK_B, 1 },
    [BUTTON_SYNC_B] = { MIDI_CH_DECK_B, 2 },
    [BUTTON_HC1_B]  = { MIDI_CH_DECK_B, 3 },
    [BUTTON_HC2_B]  = { MIDI_CH_DECK_B, 4 },
    [BUTTON_HC3_B]  = { MIDI_CH_DECK_B, 5 },
    [BUTTON_HC4_B]  = { MIDI_CH_DECK_B, 6 },

    [BUTTON_SHIFT]  = { MIDI_CH_SHARED, 0 },
};

/* High-res (14-bit) controls send MSB on `number` and LSB on `number + 32` */
static const midi_target_t analog_map[ANALOG_COUNT] = {
    [FADER_PITCH_A]    = { MIDI_CH_DECK_A, 16 },   /* + LSB on CC 48 */
    [POT_GAIN_A]       = { MIDI_CH_DECK_A, 20 },
    [POT_BASS_A]       = { MIDI_CH_DECK_A, 21 },
    [POT_MID_A]        = { MIDI_CH_DECK_A, 22 },
    [POT_TREBLE_A]     = { MIDI_CH_DECK_A, 23 },
    [POT_FX1_A]        = { MIDI_CH_DECK_A, 24 },
    [POT_FX2_A]        = { MIDI_CH_DECK_A, 25 },
    [FADER_CHANNEL_A]  = { MIDI_CH_DECK_A, 26 },

    [FADER_PITCH_B]    = { MIDI_CH_DECK_B, 16 },   /* + LSB on CC 48 */
    [POT_GAIN_B]       = { MIDI_CH_DECK_B, 20 },
    [POT_BASS_B]       = { MIDI_CH_DECK_B, 21 },
    [POT_MID_B]        = { MIDI_CH_DECK_B, 22 },
    [POT_TREBLE_B]     = { MIDI_CH_DECK_B, 23 },
    [POT_FX1_B]        = { MIDI_CH_DECK_B, 24 },
    [POT_FX2_B]        = { MIDI_CH_DECK_B, 25 },
    [FADER_CHANNEL_B]  = { MIDI_CH_DECK_B, 26 },

    [POT_MASTER]       = { MIDI_CH_SHARED, 20 },
    [FADER_CROSSFADER] = { MIDI_CH_SHARED, 21 },
};

_Static_assert(16 + 32 < 64, "14-bit pitch CC pair must stay below the switch range");

/* Marks a control as "never sent", so the next sync sends it */
#define NOT_SENT                0xFFFFU

static uint16_t analog_sent[ANALOG_COUNT];

static void forget_sent_values(void)
{
    for (uint8_t id = 0; id < ANALOG_COUNT; id++) {
        analog_sent[id] = NOT_SENT;
    }
}

void midi_map_init(void)
{
    forget_sent_values();
}

/*
 * Note messages waiting for room in the USB TX buffer, oldest first. Buttons
 * must never be lost: a missing Note Off leaves the note stuck on in the DJ
 * software. 32 entries = every button pressed and released while the host
 * reads nothing; analog controls need no queue, they are resent anyway.
 */
#define NOTE_QUEUE_LEN          32U

typedef struct {
    uint8_t channel;
    uint8_t note;
    bool    on;
} note_msg_t;

static note_msg_t note_queue[NOTE_QUEUE_LEN];
static uint8_t note_queue_head;
static uint8_t note_queue_count;

void midi_map_on_button(const button_event_t *event)
{
    const midi_target_t *t = &button_map[event->id];
    const uint8_t note = (uint8_t)(t->number + (event->shifted ? MIDI_SHIFT_NOTE_OFFSET : 0U));

    /* Not connected: nobody listens, and a stale press must not arrive later */
    if (!usb_midi_is_mounted()) {
        return;
    }

    if (note_queue_count == NOTE_QUEUE_LEN) {
#if MIDI_DEBUG_PRINT
        debug_printf("note ch%u %3u dropped, queue full\n", t->channel + 1U, note);
#endif
        return;
    }

    note_queue[(note_queue_head + note_queue_count) % NOTE_QUEUE_LEN] = (note_msg_t){
        .channel = t->channel,
        .note    = note,
        .on      = event->pressed,
    };
    note_queue_count++;
}

void midi_map_send_notes(void)
{
    if (!usb_midi_is_mounted()) {
        note_queue_count = 0;
        return;
    }

    while (note_queue_count > 0U) {
        const note_msg_t *m = &note_queue[note_queue_head];
        if (!usb_midi_send_note(m->channel, m->note, m->on, MIDI_NOTE_VELOCITY)) {
            /* USB buffer full: keep it, retry on the next call */
            return;
        }

#if MIDI_DEBUG_PRINT
        debug_printf("note ch%u %3u %s\n", m->channel + 1U, m->note, m->on ? "on" : "off");
#endif
        note_queue_head = (uint8_t)((note_queue_head + 1U) % NOTE_QUEUE_LEN);
        note_queue_count--;
    }
}

void midi_map_sync_analog(void)
{
    if (usb_midi_take_connect_event()) {
        forget_sent_values();
    }

    /* Buttons go first: while notes still wait for buffer room, keep it for them */
    if (!usb_midi_is_mounted() || !analog_is_ready() || note_queue_count > 0U) {
        return;
    }

    for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
        const analog_id_t id = (analog_id_t)i;
        const uint16_t value = analog_output(id);

        if (value == analog_sent[id]) {
            continue;
        }

        const midi_target_t *t = &analog_map[id];
        const bool sent = analog_is_high_res(id)
                        ? usb_midi_send_cc14(t->channel, t->number, value)
                        : usb_midi_send_cc(t->channel, t->number, (uint8_t)value);

        if (!sent) {
            /* USB buffer full: stop here, the rest goes out on the next scan */
            break;
        }
        analog_sent[id] = value;

#if MIDI_DEBUG_PRINT
        debug_printf("cc   ch%u %3u = %5u  %s\n", t->channel + 1U, t->number, value, analog_name(id));
#endif
    }
}
