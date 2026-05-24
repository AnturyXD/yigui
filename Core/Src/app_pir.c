#include "app_pir.h"

/* PIR状态缓存：中断更新，主循环读取 */
static volatile uint8_t g_pir_state = 0U;

void APP_PIR_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init_struct.Pin = APP_PIR_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(APP_PIR_GPIO_PORT, &gpio_init_struct);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    g_pir_state = (HAL_GPIO_ReadPin(APP_PIR_GPIO_PORT, APP_PIR_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

uint8_t APP_PIR_ReadState(void)
{
    return g_pir_state;
}

void APP_PIR_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin != APP_PIR_GPIO_PIN)
    {
        return;
    }

    g_pir_state = (HAL_GPIO_ReadPin(APP_PIR_GPIO_PORT, APP_PIR_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}
