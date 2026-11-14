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
#define ZIF10_Pin GPIO_PIN_0
#define ZIF10_GPIO_Port GPIOA
#define ZIF9_Pin GPIO_PIN_1
#define ZIF9_GPIO_Port GPIOA
#define ZIF8_Pin GPIO_PIN_2
#define ZIF8_GPIO_Port GPIOA
#define ZIF6_Pin GPIO_PIN_3
#define ZIF6_GPIO_Port GPIOA
#define ZIF5_Pin GPIO_PIN_4
#define ZIF5_GPIO_Port GPIOA
#define ZIF4_Pin GPIO_PIN_5
#define ZIF4_GPIO_Port GPIOA
#define ZIF3_Pin GPIO_PIN_6
#define ZIF3_GPIO_Port GPIOA
#define ZIF2_Pin GPIO_PIN_7
#define ZIF2_GPIO_Port GPIOA
#define ZIF1_Pin GPIO_PIN_0
#define ZIF1_GPIO_Port GPIOB
#define ZIF13_Pin GPIO_PIN_1
#define ZIF13_GPIO_Port GPIOB
#define ZIF12_Pin GPIO_PIN_2
#define ZIF12_GPIO_Port GPIOB
#define ZIF11_Pin GPIO_PIN_10
#define ZIF11_GPIO_Port GPIOB
#define DUT_EN_Pin GPIO_PIN_13
#define DUT_EN_GPIO_Port GPIOB
#define StartKey_Pin GPIO_PIN_12
#define StartKey_GPIO_Port GPIOB
#define LCD_1602_SCL_Pin GPIO_PIN_6
#define LCD_1602_SCL_GPIO_Port GPIOB
#define LCD_1602_SDA_Pin GPIO_PIN_7
#define LCD_1602_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
