#ifndef DJ_ANALOG_H
#define DJ_ANALOG_H

#include <stdint.h>

/*
 * Analog controls: 13 potentiometers + 5 sliders read through two CD74HC4067
 * multiplexers by ADC1 (see PINOUT.md).
 *
 * Enum value = index in the value table. Order matches the mux channels:
 * pots are MUX A C0..C12, faders are MUX B C0..C4.
 */
typedef enum {
    /* MUX A, SIG -> PA0 (ADC1_IN0), channels C0..C12 */
    POT_MASTER = 0,
    POT_GAIN_A,
    POT_BASS_A,
    POT_MID_A,
    POT_TREBLE_A,
    POT_FX1_A,
    POT_FX2_A,
    POT_GAIN_B,
    POT_BASS_B,
    POT_MID_B,
    POT_TREBLE_B,
    POT_FX1_B,
    POT_FX2_B,

    /* MUX B, SIG -> PA1 (ADC1_IN1), channels C0..C4 */
    FADER_PITCH_A,
    FADER_PITCH_B,
    FADER_CHANNEL_A,
    FADER_CHANNEL_B,
    FADER_CROSSFADER,

    ANALOG_COUNT
} analog_id_t;

#define ANALOG_POT_COUNT    ((uint8_t)(FADER_PITCH_A - POT_MASTER))     /* 13 */
#define ANALOG_FADER_COUNT  ((uint8_t)(ANALOG_COUNT - FADER_PITCH_A))   /* 5 */

/* Full scale of the 12-bit ADC: values are 0..ANALOG_RAW_MAX */
#define ANALOG_RAW_MAX      4095U

/*
 * Calibrates ADC1 and starts the microsecond delay timer.
 * Call once from the task that will scan, before analog_scan().
 */
void analog_init(void);

/*
 * Reads all 18 controls once: for each mux channel, select it, let the signal
 * settle, convert PA0 and (for C0..C4) PA1. Blocking, takes about 0.25 ms.
 */
void analog_scan(void);

/* Last raw value (0..ANALOG_RAW_MAX) of a control, updated by analog_scan() */
uint16_t analog_get(analog_id_t id);

/* Prints all raw values to the debug UART, for bench testing */
void analog_debug_print(void);

#endif //DJ_ANALOG_H
