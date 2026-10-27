#ifndef SIM_STM32F1XX_HAL_H
#define SIM_STM32F1XX_HAL_H

/*
 * Host simulation stand-in for the STM32 HAL: only the types, macros and
 * functions the firmware modules under test use. Implemented in sim.c,
 * where GPIO reads/writes and ADC conversions hit the simulated panel.
 */

#include <stdint.h>

typedef struct {
    char name;      /* 'A', 'B', 'C' */
} GPIO_TypeDef;

extern GPIO_TypeDef sim_gpioa, sim_gpiob, sim_gpioc;
#define GPIOA (&sim_gpioa)
#define GPIOB (&sim_gpiob)
#define GPIOC (&sim_gpioc)

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

#define GPIO_MODE_OUTPUT_PP     0x01U
#define GPIO_NOPULL             0x00U
#define GPIO_SPEED_FREQ_LOW     0x02U

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void HAL_GPIO_DeInit(GPIO_TypeDef *port, uint32_t pin);

typedef enum {
    HAL_OK = 0,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

typedef struct {
    int unused;
} ADC_HandleTypeDef;

typedef struct {
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
} ADC_ChannelConfTypeDef;

#define ADC_CHANNEL_0               0U
#define ADC_CHANNEL_1               1U
#define ADC_REGULAR_RANK_1          1U
#define ADC_SAMPLETIME_71CYCLES_5   6U

HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc, ADC_ChannelConfTypeDef *cfg);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout_ms);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *hadc);

/* Core debug unit and DWT cycle counter (delay_us in analog.c) */
typedef struct {
    uint32_t DEMCR;
} CoreDebug_Type;

typedef struct {
    uint32_t CTRL;
    uint32_t CYCCNT;
} DWT_Type;

extern CoreDebug_Type sim_coredebug;
extern DWT_Type sim_dwt;
#define CoreDebug   (&sim_coredebug)
#define DWT         (&sim_dwt)
#define CoreDebug_DEMCR_TRCENA_Msk  (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << 0)

/* 0 in the simulation, so delay_us() returns at once instead of spinning */
extern uint32_t SystemCoreClock;

typedef enum {
    USB_HP_CAN1_TX_IRQn  = 19,
    USB_LP_CAN1_RX0_IRQn = 20
} IRQn_Type;

void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t preempt, uint32_t sub);

#define __HAL_RCC_USB_CLK_ENABLE()  do { } while (0)

#endif //SIM_STM32F1XX_HAL_H
