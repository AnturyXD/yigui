#ifndef __APP_LED_H
#define __APP_LED_H

#include "main.h"
#include <stdint.h>

/* 红灯: PC5, 低电平点亮 */
#define APP_LED_RED_GPIO_PORT GPIOC
#define APP_LED_RED_GPIO_PIN  GPIO_PIN_5

/* 蓝灯: PB2, 低电平熄灭/高电平点亮(由板级电路决定) */
#define APP_LED_BLUE_GPIO_PORT GPIOB
#define APP_LED_BLUE_GPIO_PIN  GPIO_PIN_2

/**
  * @brief  LED驱动初始化函数
  * @param  无
  * @retval 无
  * @note   初始化红灯与蓝灯输出模式，并设置默认电平
  */
void APP_LED_Init(void);

/**
  * @brief  根据人体红外状态控制红灯
  * @param  pir_detected: 1-检测到人体, 0-未检测到人体
  * @retval 无
  * @note   红灯低电平点亮
  */
void APP_LED_SetPIRIndicator(uint8_t pir_detected);

/**
  * @brief  翻转蓝灯状态(心跳指示)
  * @param  无
  * @retval 无
  */
void APP_LED_ToggleHeartbeat(void);

#endif
