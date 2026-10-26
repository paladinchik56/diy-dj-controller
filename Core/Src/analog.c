/*
 * Analog controls scanner.
 *
 * Peripheral: ADC1, polling, software trigger, single conversion
 * (configured in CubeMX, adc.c). ADC clock = PCLK2 / 6 = 12 MHz (max 14 MHz).
 *   PA0 = ADC1_IN0 <- SIG of MUX A (13 potentiometers)
 *   PA1 = ADC1_IN1 <- SIG of MUX B (5 sliders)
 * Mux address on PB12..PB15, see mux.c.
 *
 * Peripheral: DWT cycle counter (Cortex-M3 core debug unit), used for
 * microsecond delays while the mux output settles.
 */

#include "analog.h"
#include "adc.h"
#include "debug_uart.h"
#include "mux.h"

_Static_assert(ANALOG_POT_COUNT == 13, "MUX A has 13 potentiometers on C0..C12");
_Static_assert(ANALOG_FADER_COUNT == 5, "MUX B has 5 sliders on C0..C4");
_Static_assert(ANALOG_POT_COUNT <= MUX_CHANNEL_COUNT, "more pots than mux inputs");

/* Time for the mux to switch and the SIG line to settle before sampling */
#define MUX_SETTLE_US           5U

/* One conversion is (71.5 + 12.5) cycles / 12 MHz = 7 us; 2 ms is "never" */
#define ADC_CONVERSION_TIMEOUT_MS  2U

/* Same as configured in CubeMX: long enough for pot + mux source impedance */
#define ADC_SAMPLING_TIME       ADC_SAMPLETIME_71CYCLES_5

static uint16_t analog_raw[ANALOG_COUNT];

static void delay_us_init(void)
{
    /* DWT: enable the trace block, reset and start the free-running cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t cycles = us * (SystemCoreClock / 1000000U);

    /* Unsigned subtraction stays correct when CYCCNT wraps past 0xFFFFFFFF */
    while ((DWT->CYCCNT - start) < cycles) {
    }
}

/*
 * One conversion on the given ADC1 input (ADC_CHANNEL_0 = PA0, ADC_CHANNEL_1 = PA1).
 * On failure *value is left untouched, so the control keeps its last reading.
 */
static void adc_read(uint32_t adc_channel, uint16_t *value)
{
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel = adc_channel;
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLING_TIME;

    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) {
        return;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return;
    }
    if (HAL_ADC_PollForConversion(&hadc1, ADC_CONVERSION_TIMEOUT_MS) != HAL_OK) {
        return;
    }

    *value = (uint16_t)HAL_ADC_GetValue(&hadc1);
}

void analog_init(void)
{
    delay_us_init();

    /* ADC1 self-calibration: measures the converter's own offset and removes it
     * from every later result. Must run once after power-up, ADC idle. */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
        Error_Handler();
    }
}

void analog_scan(void)
{
    /* MUX A uses more channels than MUX B, so its count bounds the loop */
    for (uint8_t ch = 0; ch < ANALOG_POT_COUNT; ch++) {
        mux_select(ch);
        delay_us(MUX_SETTLE_US);

        /* PA0: MUX A channel ch -> potentiometer with the same index */
        adc_read(ADC_CHANNEL_0, &analog_raw[POT_MASTER + ch]);

        /* PA1: MUX B channel ch -> slider, only C0..C4 are wired */
        if (ch < ANALOG_FADER_COUNT) {
            adc_read(ADC_CHANNEL_1, &analog_raw[FADER_PITCH_A + ch]);
        }
    }
}

uint16_t analog_get(analog_id_t id)
{
    if (id >= ANALOG_COUNT) {
        return 0;
    }
    return analog_raw[id];
}

void analog_debug_print(void)
{
    debug_printf("pots  : %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u\n",
                 analog_raw[POT_MASTER],
                 analog_raw[POT_GAIN_A], analog_raw[POT_BASS_A], analog_raw[POT_MID_A],
                 analog_raw[POT_TREBLE_A], analog_raw[POT_FX1_A], analog_raw[POT_FX2_A],
                 analog_raw[POT_GAIN_B], analog_raw[POT_BASS_B], analog_raw[POT_MID_B],
                 analog_raw[POT_TREBLE_B], analog_raw[POT_FX1_B], analog_raw[POT_FX2_B]);
    debug_printf("faders: %4u %4u %4u %4u %4u\n",
                 analog_raw[FADER_PITCH_A], analog_raw[FADER_PITCH_B],
                 analog_raw[FADER_CHANNEL_A], analog_raw[FADER_CHANNEL_B],
                 analog_raw[FADER_CROSSFADER]);
}
