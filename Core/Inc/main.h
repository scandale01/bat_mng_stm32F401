/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f4xx_hal.h"

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
#define OUT_9_Pin GPIO_PIN_13
#define OUT_9_GPIO_Port GPIOC
#define LED_8_Pin GPIO_PIN_0
#define LED_8_GPIO_Port GPIOC
#define LED_6_Pin GPIO_PIN_1
#define LED_6_GPIO_Port GPIOC
#define OUT_4_Pin GPIO_PIN_2
#define OUT_4_GPIO_Port GPIOC
#define LED_7_Pin GPIO_PIN_3
#define LED_7_GPIO_Port GPIOC
#define ADC_1_Pin GPIO_PIN_0
#define ADC_1_GPIO_Port GPIOA
#define ADC_2_Pin GPIO_PIN_1
#define ADC_2_GPIO_Port GPIOA
#define LED_1_Pin GPIO_PIN_2
#define LED_1_GPIO_Port GPIOA
#define LED_2_Pin GPIO_PIN_3
#define LED_2_GPIO_Port GPIOA
#define LED_4_Pin GPIO_PIN_4
#define LED_4_GPIO_Port GPIOA
#define IN_2_Pin GPIO_PIN_5
#define IN_2_GPIO_Port GPIOA
#define IN_4_Pin GPIO_PIN_6
#define IN_4_GPIO_Port GPIOA
#define IN_6_Pin GPIO_PIN_7
#define IN_6_GPIO_Port GPIOA
#define IN_1_Pin GPIO_PIN_5
#define IN_1_GPIO_Port GPIOC
#define Relay_PWM_Pin GPIO_PIN_0
#define Relay_PWM_GPIO_Port GPIOB
#define LED_3_Pin GPIO_PIN_1
#define LED_3_GPIO_Port GPIOB
#define LED_5_Pin GPIO_PIN_2
#define LED_5_GPIO_Port GPIOB
#define IN_7_Pin GPIO_PIN_12
#define IN_7_GPIO_Port GPIOB
#define IN_3_OPTO_Pin GPIO_PIN_13
#define IN_3_OPTO_GPIO_Port GPIOB
#define IN_1_OPTO_Pin GPIO_PIN_14
#define IN_1_OPTO_GPIO_Port GPIOB
#define IN_9_Pin GPIO_PIN_7
#define IN_9_GPIO_Port GPIOC
#define IN_10_Pin GPIO_PIN_9
#define IN_10_GPIO_Port GPIOA
#define IN_5_Pin GPIO_PIN_11
#define IN_5_GPIO_Port GPIOA
#define IN_3_Pin GPIO_PIN_12
#define IN_3_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define OUT_5_Pin GPIO_PIN_15
#define OUT_5_GPIO_Port GPIOA
#define OUT_1_Pin GPIO_PIN_10
#define OUT_1_GPIO_Port GPIOC
#define OUT_2_Pin GPIO_PIN_11
#define OUT_2_GPIO_Port GPIOC
#define OUT_3_Pin GPIO_PIN_12
#define OUT_3_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define IN_2_OPTO_Pin GPIO_PIN_5
#define IN_2_OPTO_GPIO_Port GPIOB
#define IN_8_Pin GPIO_PIN_6
#define IN_8_GPIO_Port GPIOB
#define OUT_10_Pin GPIO_PIN_7
#define OUT_10_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
