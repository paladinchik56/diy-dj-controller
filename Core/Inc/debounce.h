#ifndef DJ_DEBOUNCE_H
#define DJ_DEBOUNCE_H

/*
 * Contact debouncer for one button. Pure logic, no HAL: can be unit-tested
 * on the host PC.
 *
 * A mechanical contact bounces for a few milliseconds when pressed or
 * released, so one press looks like several. The stable state flips only
 * after the raw reading has disagreed with it for `samples` reads in a row;
 * any agreeing read in between restarts the count.
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool    stable;     /* debounced state, true = pressed */
    uint8_t count;      /* consecutive reads that disagree with `stable` */
    uint8_t samples;    /* reads needed to accept a change */
} debounce_t;

void debounce_init(debounce_t *d, uint8_t samples);

/* Feeds one raw read (true = pressed). Returns true when the stable state flipped. */
bool debounce_update(debounce_t *d, bool raw_pressed);

#endif //DJ_DEBOUNCE_H
