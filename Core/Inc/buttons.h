#ifndef DJ_BUTTONS_H
#define DJ_BUTTONS_H

#include <stdbool.h>

/*
 * Panel buttons (see PINOUT_BY_PIN.md). Each button connects its GPIO to GND,
 * the internal pull-up keeps the pin high otherwise: pressed = low.
 *
 * Names use the BUTTON_ prefix so they never clash with the CubeMX pin labels
 * (BTN_PLAY_A_Pin, ...) in main.h.
 */
typedef enum {
    BUTTON_PLAY_A = 0,  /* PA2  */
    BUTTON_CUE_A,       /* PA3  */
    BUTTON_SYNC_A,      /* PA4  */
    BUTTON_SHIFT,       /* PA5  */
    BUTTON_PLAY_B,      /* PA6  */
    BUTTON_CUE_B,       /* PA7  */
    BUTTON_SYNC_B,      /* PB3  */
    BUTTON_HC1_A,       /* PB4  */
    BUTTON_HC2_A,       /* PB5  */
    BUTTON_HC3_A,       /* PB6  */
    BUTTON_HC4_A,       /* PB7  */
    BUTTON_HC1_B,       /* PB8  */
    BUTTON_HC2_B,       /* PB9  */
    BUTTON_HC3_B,       /* PB10 */
    BUTTON_HC4_B,       /* PB11 */

    BUTTON_COUNT
} button_id_t;

/*
 * One debounced press or release.
 * shifted: Shift was held when this button was pressed. A release always
 * repeats the value of its press, even if Shift was let go in between, so a
 * press and its release always map to the same MIDI note (no stuck notes).
 * Always false for the Shift button itself.
 */
typedef struct {
    button_id_t id;
    bool        pressed;
    bool        shifted;
} button_event_t;

typedef void (*button_event_cb_t)(const button_event_t *event);

void buttons_init(void);

/*
 * Reads all buttons once and calls on_event for every debounced change.
 * Call every 1 ms: a change is accepted after BUTTONS_DEBOUNCE_MS stable reads.
 */
void buttons_scan(button_event_cb_t on_event);

/* Debounced state of a button, true = pressed */
bool buttons_is_pressed(button_id_t id);

/* Short button name for debug output, e.g. "PLAY_A" */
const char *buttons_name(button_id_t id);

#endif //DJ_BUTTONS_H
