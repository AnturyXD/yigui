#ifndef __APP_PIR_H
#define __APP_PIR_H

#include "main.h"
#include <stdint.h>

/* 人体红外输入: PA1 */
#define APP_PIR_GPIO_PORT GPIOA
#define APP_PIR_GPIO_PIN  GPIO_PIN_1

/**
  * @brief  人体红外驱动初始化函数
  * @param  无
  * @retval 无
  * @note   配置PA1为普通输入
  */
void APP_PIR_Init(void);

/**
  * @brief  读取人体红外状态
  * @param  无
  * @retval 1-检测到人体, 0-未检测到人体
  */
uint8_t APP_PIR_ReadState(void);

/**
  * @brief  人体红外中断回调
  * @param  gpio_pin: 触发中断的引脚编号
  * @retval 无
  */
void APP_PIR_EXTI_Callback(uint16_t gpio_pin);

#endif
