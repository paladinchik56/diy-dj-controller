/*
 * Panel buttons scanner.
 *
 * Peripheral: GPIO input with internal pull-up (configured in CubeMX, gpio.c):
 *   PA2..PA7  Play A, Cue A, Sync A, Shift, Play B, Cue B
 *   PB3..PB11 Sync B, Hotcue 1-4 A, Hotcue 1-4 B
 * PB3/PB4 are JTAG pins after reset; HAL_MspInit() switches JTAG off
 * (SWD stays on), which frees them as GPIO.
 */

#include "buttons.h"
#include "debounce.h"
#include "main.h"

/* Number of consecutive 1 ms reads that must agree before a change is accepted */
#define BUTTONS_DEBOUNCE_MS     5U

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    const char   *name;
} button_pin_t;

/* Indexed by button_id_t, pins taken from the CubeMX labels in main.h */
static const button_pin_t button_pins[BUTTON_COUNT] = {
    [BUTTON_PLAY_A] = { BTN_PLAY_A_GPIO_Port, BTN_PLAY_A_Pin, "PLAY_A" },
    [BUTTON_CUE_A]  = { BTN_CUE_A_GPIO_Port,  BTN_CUE_A_Pin,  "CUE_A"  },
    [BUTTON_SYNC_A] = { BTN_SYNC_A_GPIO_Port, BTN_SYNC_A_Pin, "SYNC_A" },
    [BUTTON_SHIFT]  = { BTN_SHIFT_GPIO_Port,  BTN_SHIFT_Pin,  "SHIFT"  },
    [BUTTON_PLAY_B] = { BTN_PLAY_B_GPIO_Port, BTN_PLAY_B_Pin, "PLAY_B" },
    [BUTTON_CUE_B]  = { BTN_CUE_B_GPIO_Port,  BTN_CUE_B_Pin,  "CUE_B"  },
    [BUTTON_SYNC_B] = { BTN_SYNC_B_GPIO_Port, BTN_SYNC_B_Pin, "SYNC_B" },
    [BUTTON_HC1_A]  = { BTN_HC1_A_GPIO_Port,  BTN_HC1_A_Pin,  "HC1_A"  },
    [BUTTON_HC2_A]  = { BTN_HC2_A_GPIO_Port,  BTN_HC2_A_Pin,  "HC2_A"  },
    [BUTTON_HC3_A]  = { BTN_HC3_A_GPIO_Port,  BTN_HC3_A_Pin,  "HC3_A"  },
    [BUTTON_HC4_A]  = { BTN_HC4_A_GPIO_Port,  BTN_HC4_A_Pin,  "HC4_A"  },
    [BUTTON_HC1_B]  = { BTN_HC1_B_GPIO_Port,  BTN_HC1_B_Pin,  "HC1_B"  },
    [BUTTON_HC2_B]  = { BTN_HC2_B_GPIO_Port,  BTN_HC2_B_Pin,  "HC2_B"  },
    [BUTTON_HC3_B]  = { BTN_HC3_B_GPIO_Port,  BTN_HC3_B_Pin,  "HC3_B"  },
    [BUTTON_HC4_B]  = { BTN_HC4_B_GPIO_Port,  BTN_HC4_B_Pin,  "HC4_B"  },
};

static debounce_t button_debounce[BUTTON_COUNT];

/* Shift state latched when each button was pressed, repeated on its release */
static bool button_shifted[BUTTON_COUNT];

void buttons_init(void)
{
    for (uint8_t id = 0; id < BUTTON_COUNT; id++) {
        debounce_init(&button_debounce[id], BUTTONS_DEBOUNCE_MS);
        button_shifted[id] = false;
    }
}

void buttons_scan(button_event_cb_t on_event)
{
    for (uint8_t id = 0; id < BUTTON_COUNT; id++) {
        /* Pull-up: idle pin reads high, a pressed button pulls it to GND */
        const bool raw_pressed =
            HAL_GPIO_ReadPin(button_pins[id].port, button_pins[id].pin) == GPIO_PIN_RESET;

        if (!debounce_update(&button_debounce[id], raw_pressed)) {
            continue;
        }

        button_event_t event = {
            .id      = (button_id_t)id,
            .pressed = button_debounce[id].stable,
        };

        if (id != BUTTON_SHIFT) {
            if (event.pressed) {
                button_shifted[id] = buttons_is_pressed(BUTTON_SHIFT);
            }
            event.shifted = button_shifted[id];
        }

        on_event(&event);
    }
}

bool buttons_is_pressed(button_id_t id)
{
    if (id >= BUTTON_COUNT) {
        return false;
    }
    return button_debounce[id].stable;
}

const char *buttons_name(button_id_t id)
{
    if (id >= BUTTON_COUNT) {
        return "?";
    }
    return button_pins[id].name;
}
