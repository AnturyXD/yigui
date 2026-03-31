#include "app_pir.h"

/**
  * @brief  人体红外驱动初始化函数
  * @param  无
  * @retval 无
  * @note   配置PA1为普通输入
  */
void APP_PIR_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 使能GPIOA时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA1：普通输入，不上拉不下拉 */
    gpio_init_struct.Pin = APP_PIR_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(APP_PIR_GPIO_PORT, &gpio_init_struct);
}

/**
  * @brief  读取人体红外状态
  * @param  无
  * @retval 1-检测到人体, 0-未检测到人体
  */
uint8_t APP_PIR_ReadState(void)
{
    if (HAL_GPIO_ReadPin(APP_PIR_GPIO_PORT, APP_PIR_GPIO_PIN) == GPIO_PIN_SET)
    {
        return 1U;
    }

    return 0U;
}
