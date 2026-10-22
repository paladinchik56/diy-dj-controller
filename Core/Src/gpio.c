/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, MUX_S0_Pin|MUX_S1_Pin|MUX_S2_Pin|MUX_S3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_PLAY_A_Pin BTN_CUE_A_Pin BTN_SYNC_A_Pin BTN_SHIFT_Pin
                           BTN_PLAY_B_Pin BTN_CUE_B_Pin */
  GPIO_InitStruct.Pin = BTN_PLAY_A_Pin|BTN_CUE_A_Pin|BTN_SYNC_A_Pin|BTN_SHIFT_Pin
                          |BTN_PLAY_B_Pin|BTN_CUE_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_HC3_B_Pin BTN_HC4_B_Pin BTN_SYNC_B_Pin BTN_HC1_A_Pin
                           BTN_HC2_A_Pin BTN_HC3_A_Pin BTN_HC4_A_Pin BTN_HC1_B_Pin
                           BTN_HC2_B_Pin */
  GPIO_InitStruct.Pin = BTN_HC3_B_Pin|BTN_HC4_B_Pin|BTN_SYNC_B_Pin|BTN_HC1_A_Pin
                          |BTN_HC2_A_Pin|BTN_HC3_A_Pin|BTN_HC4_A_Pin|BTN_HC1_B_Pin
                          |BTN_HC2_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : MUX_S0_Pin MUX_S1_Pin MUX_S2_Pin MUX_S3_Pin */
  GPIO_InitStruct.Pin = MUX_S0_Pin|MUX_S1_Pin|MUX_S2_Pin|MUX_S3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
