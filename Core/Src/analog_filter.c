#include "analog_filter.h"

#define RAW_MAX             4095
#define FRAC_BITS           4           /* smoothed values are raw * 16 */
#define SCALE               (1 << FRAC_BITS)

/* EMA weight 1/8: each reading moves the result by 1/8 of the difference.
 * Good against noise, but alone it takes ~150 ms to follow a full-range jump. */
#define EMA_DIVISOR         8

/* A jump larger than this is a hand moving the control, not noise (noise is
 * about +-6 raw units): follow it at once, so crossfader cuts have no lag. */
#define FAST_MOVE_RAW       32

/* After power-up the first reading is as noisy as any other. For this many
 * readings the deadband anchor silently follows the average while it forms;
 * anchoring on that single first sample would let noise cross the deadband
 * seconds later and send a change from a control nobody touched. */
#define SETTLE_READINGS     (2 * EMA_DIVISOR)

/* Readings this close to either end count as fully off / fully on, because
 * wiring resistance keeps real pots from reaching exactly 0 or 4095. */
#define END_DEAD_ZONE_RAW   24

#define IN_LOW              (END_DEAD_ZONE_RAW * SCALE)
#define IN_HIGH             ((RAW_MAX - END_DEAD_ZONE_RAW) * SCALE)

static uint16_t scale_to_output(int32_t smoothed, uint16_t out_max)
{
    if (smoothed <= IN_LOW) {
        return 0;
    }
    if (smoothed >= IN_HIGH) {
        return out_max;
    }

    const uint32_t span = (uint32_t)(IN_HIGH - IN_LOW);
    const uint32_t pos  = (uint32_t)(smoothed - IN_LOW);

    /* + span / 2 rounds to the nearest step instead of truncating.
     * Worst case pos * out_max = 65136 * 16383, still fits in 32 bits. */
    return (uint16_t)((pos * out_max + span / 2U) / span);
}

void analog_filter_init(analog_filter_t *f, uint16_t out_max, uint16_t hysteresis)
{
    f->smoothed   = 0;
    f->anchor     = 0;
    f->out        = 0;
    f->out_max    = out_max;
    f->hysteresis = hysteresis;
    f->primed     = false;
    f->settling   = 0;
}

bool analog_filter_update(analog_filter_t *f, uint16_t raw, uint16_t *out)
{
    const int32_t sample = (int32_t)raw * SCALE;

    if (!f->primed) {
        /* Start from the real position instead of ramping up from 0,
         * otherwise every control would "move" right after power-up. */
        f->smoothed = sample;
        f->anchor   = sample;
        f->out      = scale_to_output(sample, f->out_max);
        f->primed   = true;
        f->settling = SETTLE_READINGS;
        *out = f->out;
        return true;
    }

    const int32_t diff = sample - f->smoothed;
    if (diff > FAST_MOVE_RAW * SCALE || diff < -FAST_MOVE_RAW * SCALE) {
        f->smoothed = sample;
        f->settling = 0;    /* a real move: report it now, not after settling */
    } else {
        f->smoothed += diff / EMA_DIVISOR;
    }

    if (f->settling > 0U) {
        f->settling--;
        f->anchor = f->smoothed;
        if (f->settling > 0U) {
            return false;
        }
        /* Settling done: fall through once to report a corrected value */
    } else {
        int32_t moved = f->smoothed - f->anchor;
        if (moved < 0) {
            moved = -moved;
        }
        if (moved <= (int32_t)f->hysteresis * SCALE) {
            return false;
        }

        f->anchor = f->smoothed;
    }

    const uint16_t new_out = scale_to_output(f->smoothed, f->out_max);
    if (new_out == f->out) {
        return false;
    }

    f->out = new_out;
    *out = new_out;
    return true;
}
