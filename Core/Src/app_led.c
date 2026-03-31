#include "app_led.h"

/**
  * @brief  LED驱动初始化函数
  * @param  无
  * @retval 无
  * @note   配置PC5和PB2为推挽输出
  */
void APP_LED_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 使能LED对应GPIO端口时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置红灯默认熄灭(低电平点亮，因此先置高) */
    HAL_GPIO_WritePin(APP_LED_RED_GPIO_PORT, APP_LED_RED_GPIO_PIN, GPIO_PIN_SET);

    /* 配置蓝灯默认熄灭(根据板级电路，这里默认先置低) */
    HAL_GPIO_WritePin(APP_LED_BLUE_GPIO_PORT, APP_LED_BLUE_GPIO_PIN, GPIO_PIN_RESET);

    /* 红灯引脚模式配置 */
    gpio_init_struct.Pin = APP_LED_RED_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(APP_LED_RED_GPIO_PORT, &gpio_init_struct);

    /* 蓝灯引脚模式配置 */
    gpio_init_struct.Pin = APP_LED_BLUE_GPIO_PIN;
    HAL_GPIO_Init(APP_LED_BLUE_GPIO_PORT, &gpio_init_struct);
}

/**
  * @brief  根据人体红外状态控制红灯
  * @param  pir_detected: 1-检测到人体, 0-未检测到人体
  * @retval 无
  * @note   红灯低电平点亮
  */
void APP_LED_SetPIRIndicator(uint8_t pir_detected)
{
    if (pir_detected == 1U)
    {
        HAL_GPIO_WritePin(APP_LED_RED_GPIO_PORT, APP_LED_RED_GPIO_PIN, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(APP_LED_RED_GPIO_PORT, APP_LED_RED_GPIO_PIN, GPIO_PIN_SET);
    }
}

/**
  * @brief  翻转蓝灯状态(心跳指示)
  * @param  无
  * @retval 无
  */
void APP_LED_ToggleHeartbeat(void)
{
    HAL_GPIO_TogglePin(APP_LED_BLUE_GPIO_PORT, APP_LED_BLUE_GPIO_PIN);
}
