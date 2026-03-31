#include "bsp_Key.h"

/* 按键事件标志：中断置1，主循环读取后清零 */
static volatile uint8_t g_key_event_flag = 0U;

/**
  * @brief  按键驱动初始化函数
  * @param  无
  * @retval 无
  * @note   配置PA0为上升沿外部中断输入
  */
void Key_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 使能按键GPIO端口时钟 */
    KEY_1_GPIO_CLK_ENABLE();

    /* 配置PA0为中断输入，闲时下拉，按下高电平 */
    gpio_init_struct.Pin = KEY_1_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_1_GPIO_PORT, &gpio_init_struct);

    /* 配置并使能EXTI0中断 */
    HAL_NVIC_SetPriority(KEY_1_EXTI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(KEY_1_EXTI_IRQn);
}

/**
  * @brief  按键中断回调处理函数
  * @param  gpio_pin: 触发中断的引脚编号
  * @retval 无
  * @note   在中断中只置位事件标志
  */
void Key_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == KEY_1_GPIO_PIN)
    {
        g_key_event_flag = 1U;
    }
}

/**
  * @brief  获取按键事件并清零
  * @param  无
  * @retval 1-检测到按键事件, 0-无按键事件
  */
uint8_t Key_GetEventAndClear(void)
{
    uint8_t key_event;

    key_event = g_key_event_flag;
    g_key_event_flag = 0U;

    return key_event;
}
