/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   GPIO基础初始化文件
  ******************************************************************************
  */
/* USER CODE END Header */

#include "gpio.h"

/**
  * @brief  GPIO基础初始化函数
  * @param  无
  * @retval 无
  * @note   本函数仅做GPIO端口时钟基础使能。
  *         各外设引脚模式由对应独立驱动文件自行初始化。
  */
void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}
