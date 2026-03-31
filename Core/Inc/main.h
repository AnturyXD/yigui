/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : 主头文件（全局引脚与接口定义）
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/
#define KEY_1_Pin GPIO_PIN_0
#define KEY_1_GPIO_Port GPIOA
#define KEY_1_EXTI_IRQn EXTI0_IRQn

#define KEY_2_Pin GPIO_PIN_1
#define KEY_2_GPIO_Port GPIOA
#define KEY_2_EXTI_IRQn EXTI1_IRQn

#define IO2_Pin GPIO_PIN_2
#define IO2_GPIO_Port GPIOA

#define IO3_Pin GPIO_PIN_3
#define IO3_GPIO_Port GPIOA

#define KEY_3_Pin GPIO_PIN_4
#define KEY_3_GPIO_Port GPIOA
#define KEY_3_EXTI_IRQn EXTI4_IRQn

#define LED_RED_Pin GPIO_PIN_5
#define LED_RED_GPIO_Port GPIOC

#define LED_BLUE_Pin GPIO_PIN_2
#define LED_BLUE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* 人体红外模块（PIR）输入引脚 */
#define PIR_Pin GPIO_PIN_1
#define PIR_GPIO_Port GPIOA
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
