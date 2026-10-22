/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define BTN_PLAY_A_Pin GPIO_PIN_2
#define BTN_PLAY_A_GPIO_Port GPIOA
#define BTN_CUE_A_Pin GPIO_PIN_3
#define BTN_CUE_A_GPIO_Port GPIOA
#define BTN_SYNC_A_Pin GPIO_PIN_4
#define BTN_SYNC_A_GPIO_Port GPIOA
#define BTN_SHIFT_Pin GPIO_PIN_5
#define BTN_SHIFT_GPIO_Port GPIOA
#define BTN_PLAY_B_Pin GPIO_PIN_6
#define BTN_PLAY_B_GPIO_Port GPIOA
#define BTN_CUE_B_Pin GPIO_PIN_7
#define BTN_CUE_B_GPIO_Port GPIOA
#define BTN_HC3_B_Pin GPIO_PIN_10
#define BTN_HC3_B_GPIO_Port GPIOB
#define BTN_HC4_B_Pin GPIO_PIN_11
#define BTN_HC4_B_GPIO_Port GPIOB
#define MUX_S0_Pin GPIO_PIN_12
#define MUX_S0_GPIO_Port GPIOB
#define MUX_S1_Pin GPIO_PIN_13
#define MUX_S1_GPIO_Port GPIOB
#define MUX_S2_Pin GPIO_PIN_14
#define MUX_S2_GPIO_Port GPIOB
#define MUX_S3_Pin GPIO_PIN_15
#define MUX_S3_GPIO_Port GPIOB
#define BTN_SYNC_B_Pin GPIO_PIN_3
#define BTN_SYNC_B_GPIO_Port GPIOB
#define BTN_HC1_A_Pin GPIO_PIN_4
#define BTN_HC1_A_GPIO_Port GPIOB
#define BTN_HC2_A_Pin GPIO_PIN_5
#define BTN_HC2_A_GPIO_Port GPIOB
#define BTN_HC3_A_Pin GPIO_PIN_6
#define BTN_HC3_A_GPIO_Port GPIOB
#define BTN_HC4_A_Pin GPIO_PIN_7
#define BTN_HC4_A_GPIO_Port GPIOB
#define BTN_HC1_B_Pin GPIO_PIN_8
#define BTN_HC1_B_GPIO_Port GPIOB
#define BTN_HC2_B_Pin GPIO_PIN_9
#define BTN_HC2_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
