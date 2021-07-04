/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bq34110.h"
extern "C"
{
#include "ssd1306.h"
#include "logger.h"
}
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t dataRead;
uint16_t gpioFlag = 0;
uint16_t calibCurtVal;
uint16_t calibVolttVal;
RTC_TimeTypeDef sysTime = {0};
RTC_DateTypeDef sysDate = {0};
uint32_t daysForTest = 7;
constexpr uint32_t checkDelay_ms = 1000;
uint32_t lasCheckTime = 0;
uint32_t testsCounter = 0;
char outputData[15];
uint32_t lcdUpdateTime = 0;
#define LOGGER_ENABLE 1
static uint32_t time;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  logInit(LOG_INFO);
//  bq.enterCalMode();
//  if(!bq.calibRawCurr(calibCurtVal)) {
////    Error_Handler();
//    calibCurtVal = 0;
//  }
//  if (!bq.calibRawVoltage(calibVolttVal)) {
//    calibVolttVal = 0;
//  }
//  bq.exitCalMode();
  bq34110::bq34 bq;
  ssd1306_Init(&hi2c1);

  HAL_Delay(1000);

  HAL_RTC_GetTime(&hrtc, &sysTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sysDate, RTC_FORMAT_BIN);
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen(&hi2c1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (bq.isVoltNorm() && !bq.m_batStatus.SOCLow) {
      if((sysDate.Date > bq.m_sysData.testCyclePeriod_days ||
          testsCounter == 0) && !bq.isTestStarted()) {
        //@TODO Add relaxation pause before discharge
          bq.startTest();
          sysTime = {0};
          sysDate = {0};
          HAL_RTC_SetTime(&hrtc, &sysTime, RTC_FORMAT_BIN);
          HAL_RTC_SetDate(&hrtc, &sysDate, RTC_FORMAT_BIN);
        }
    }
    if(bq.isTestStarted() && (HAL_GetTick() - lasCheckTime > checkDelay_ms)) {
      bq.updBatCondData();
      HAL_Delay(200);
      bq.updBatStatus();
      bq.checkTestCondition(testsCounter);

    }
//    bq.updEOSLearnStatus();

    //for testing
    if((HAL_GetTick() - lcdUpdateTime) > 1000) {
      ssd1306_Fill(Black);
      ssd1306_UpdateScreen(&hi2c1);
      ssd1306_SetCursor(0, 0);
      if(bq.isTestStarted()) {
        ssd1306_WriteString("Test ON", Font_11x18, White);
        ssd1306_SetCursor(0, 12);
        snprintf(outputData, sizeof(outputData), "Cap mAh: %d", bq.m_batCond.acummCharge);
        ssd1306_WriteString(outputData, Font_7x10, White);
      } else {
        ssd1306_WriteString("Test OFF", Font_11x18, White);
        ssd1306_SetCursor(0, 20);
        snprintf(outputData, sizeof(outputData), "Voltage: %d", bq.getVoltage());
        ssd1306_WriteString(outputData, Font_7x10, White);
        ssd1306_SetCursor(0, 32);
        if (bq.m_batStatus.SOCLow) {
          ssd1306_WriteString("SOCLow: True", Font_7x10, White);
        } else {
          ssd1306_WriteString("SOCLow: False", Font_7x10, White);
        }
      }
      time =HAL_GetTick();
      ssd1306_UpdateScreen(&hi2c1);
      lcdUpdateTime = HAL_GetTick();
      time = lcdUpdateTime - time;
      logInfo("Update time: %d", lcdUpdateTime-time);
    }
    HAL_Delay(100);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if(GPIO_Pin == B1_Pin) {
    gpioFlag = 1;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
