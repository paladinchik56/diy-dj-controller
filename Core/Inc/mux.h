//
// Created by paladinchik56 on 15.10.2026.
//

#ifndef DJ_MUX_H
#define DJ_MUX_H

#include <stdint.h>

/* Number of inputs on one CD74HC4067 (C0..C15) */
#define MUX_CHANNEL_COUNT 16U

/*
 * Peripheral: GPIO, outputs PB12..PB15 -> S0..S3 of both CD74HC4067 multiplexers.
 * MUX A (SIG -> PA0, 13 potentiometers) and MUX B (SIG -> PA1, 5 faders)
 * share the address bus, so they switch together.
 *
 * Puts channel (0..15) on the address bus in binary, S0 = least significant bit.
 * Values >= MUX_CHANNEL_COUNT are ignored (address stays unchanged).
 * After the call, give the SIG output time to settle before an ADC conversion.
 */
void mux_select(uint8_t channel);

#endif //DJ_MUX_H
