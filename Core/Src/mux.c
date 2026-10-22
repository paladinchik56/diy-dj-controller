//
// Created by paladinchik56 on 15.10.2026.
//

#include "mux.h"
#include "main.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} mux_addr_pin_t;

/*
 * Peripheral: GPIO port B, output push-pull, low speed (configured in CubeMX, gpio.c).
 * Array index = address bit number: [0] = S0 (PB12) ... [3] = S3 (PB15).
 */
static const mux_addr_pin_t mux_addr_pins[] = {
    { MUX_S0_GPIO_Port, MUX_S0_Pin },   /* PB12 -> S0, bit 0 */
    { MUX_S1_GPIO_Port, MUX_S1_Pin },   /* PB13 -> S1, bit 1 */
    { MUX_S2_GPIO_Port, MUX_S2_Pin },   /* PB14 -> S2, bit 2 */
    { MUX_S3_GPIO_Port, MUX_S3_Pin },   /* PB15 -> S3, bit 3 */
};

#define MUX_ADDR_BITS (sizeof(mux_addr_pins) / sizeof(mux_addr_pins[0]))

void mux_select(uint8_t channel)
{
    if (channel >= MUX_CHANNEL_COUNT) {
        return;
    }

    for (int bitPosition = MUX_ADDR_BITS - 1; bitPosition >= 0; bitPosition--) {
        uint8_t bit = (channel >> bitPosition) & 1;

        HAL_GPIO_WritePin(mux_addr_pins[bitPosition].port,
                          mux_addr_pins[bitPosition].pin,
                          bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
