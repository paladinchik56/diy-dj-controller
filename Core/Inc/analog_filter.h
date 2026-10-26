#ifndef DJ_ANALOG_FILTER_H
#define DJ_ANALOG_FILTER_H

/*
 * Turns noisy 12-bit ADC readings of one control into a stable output value.
 * Pure arithmetic, no HAL: can be unit-tested on the host PC.
 *
 * Pipeline per reading:
 *   1. smoothing:  exponential moving average, kept with 4 fractional bits;
 *                  big jumps (a hand moving the control) are followed at once
 *   2. deadband:   output is recomputed only when the smoothed value moved by
 *                  more than `hysteresis` raw units since the last report,
 *                  so a resting control never flips between two values
 *   3. scaling:    dead zones at both ends (so 0 and out_max are always
 *                  reachable), the rest stretched linearly to 0..out_max
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t  smoothed;      /* raw value * 16 (4 fractional bits) */
    int32_t  anchor;        /* smoothed value at the last recompute, same scale */
    uint16_t out;           /* last reported output, 0..out_max */
    uint16_t out_max;       /* 127 for 7-bit MIDI, 16383 for 14-bit */
    uint16_t hysteresis;    /* deadband in raw ADC units */
    bool     primed;        /* false until the first reading */
} analog_filter_t;

void analog_filter_init(analog_filter_t *f, uint16_t out_max, uint16_t hysteresis);

/*
 * Feeds one raw reading (0..4095). Returns true and writes *out when the
 * output value changed (the first reading always counts as a change).
 */
bool analog_filter_update(analog_filter_t *f, uint16_t raw, uint16_t *out);

#endif //DJ_ANALOG_FILTER_H
