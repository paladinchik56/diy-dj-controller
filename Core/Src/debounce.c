#include "debounce.h"

void debounce_init(debounce_t *d, uint8_t samples)
{
    d->stable  = false;
    d->count   = 0;
    d->samples = samples;
}

bool debounce_update(debounce_t *d, bool raw_pressed)
{
    if (raw_pressed == d->stable) {
        d->count = 0;
        return false;
    }

    d->count++;
    if (d->count < d->samples) {
        return false;
    }

    d->stable = raw_pressed;
    d->count  = 0;
    return true;
}
