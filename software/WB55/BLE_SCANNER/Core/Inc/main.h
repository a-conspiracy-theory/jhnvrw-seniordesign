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
#include "stm32wbxx_hal.h"
#include "app_conf.h"
#include "app_entry.h"
#include "app_common.h"

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
#define SX1262_DIO1_Pin GPIO_PIN_0
#define SX1262_DIO1_GPIO_Port GPIOA
#define SX1262_BUSY_Pin GPIO_PIN_2
#define SX1262_BUSY_GPIO_Port GPIOA
#define SX1262_DIO2_CPS_Pin GPIO_PIN_3
#define SX1262_DIO2_CPS_GPIO_Port GPIOA
#define SX1262_RST_Pin GPIO_PIN_4
#define SX1262_RST_GPIO_Port GPIOA
#define SX1262_TXEN_Pin GPIO_PIN_5
#define SX1262_TXEN_GPIO_Port GPIOA
#define SX1262_NSS_Pin GPIO_PIN_2
#define SX1262_NSS_GPIO_Port GPIOB
#define BTN5_Pin GPIO_PIN_15
#define BTN5_GPIO_Port GPIOA
#define BTN2_Pin GPIO_PIN_3
#define BTN2_GPIO_Port GPIOB
#define BTN1_Pin GPIO_PIN_4
#define BTN1_GPIO_Port GPIOB
#define BTN3_Pin GPIO_PIN_5
#define BTN3_GPIO_Port GPIOB
#define BTN4_Pin GPIO_PIN_6
#define BTN4_GPIO_Port GPIOB
#define SX1262_RXEN_Pin GPIO_PIN_7
#define SX1262_RXEN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
