#ifndef _BSP_KEY_H
#define _BSP_KEY_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/******************************************************************************
 * 文件名称: bsp_Key.h
 * 功能说明: 按键驱动头文件（BSP层）
 * 说明    :
 *           1. 当前使用 KEY_1（PA0）作为外部中断按键
 *           2. 中断中只置位标志，主循环读取事件
 ******************************************************************************/

/* KEY_1 硬件定义：PA0，闲时下拉，按下为高电平，上升沿触发 */
#define KEY_1_GPIO_PORT      GPIOA
#define KEY_1_GPIO_PIN       GPIO_PIN_0
#define KEY_1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define KEY_1_EXTI_IRQn      EXTI0_IRQn

/**
  * @brief  按键驱动初始化函数
  * @param  无
  * @retval 无
  * @note   配置PA0为上升沿外部中断输入
  */
void Key_Init(void);

/**
  * @brief  按键中断回调处理函数
  * @param  gpio_pin: 触发中断的引脚编号
  * @retval 无
  * @note   在中断回调中只置位按键事件标志
  */
void Key_EXTI_Callback(uint16_t gpio_pin);

/**
  * @brief  获取按键事件并清零
  * @param  无
  * @retval 1-检测到按键事件, 0-无按键事件
  */
uint8_t Key_GetEventAndClear(void);

#endif
