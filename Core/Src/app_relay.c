#include "app_relay.h"

/* 灯状态缓存: 1=开灯, 0=关灯 */
static volatile uint8_t g_light_on_state = 0U;

void APP_RELAY_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 默认关灯: 低电平触发，因此默认输出高电平 */
    HAL_GPIO_WritePin(APP_RELAY_GPIO_PORT, APP_RELAY_GPIO_PIN, GPIO_PIN_SET);

    gpio_init_struct.Pin = APP_RELAY_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(APP_RELAY_GPIO_PORT, &gpio_init_struct);

    g_light_on_state = 0U;
}

void APP_RELAY_SetLightState(uint8_t light_on)
{
    if (light_on == 1U)
    {
        HAL_GPIO_WritePin(APP_RELAY_GPIO_PORT, APP_RELAY_GPIO_PIN, GPIO_PIN_RESET);
        g_light_on_state = 1U;
    }
    else
    {
        HAL_GPIO_WritePin(APP_RELAY_GPIO_PORT, APP_RELAY_GPIO_PIN, GPIO_PIN_SET);
        g_light_on_state = 0U;
    }
}

void APP_RELAY_ToggleLight(void)
{
    if (g_light_on_state == 1U)
    {
        APP_RELAY_SetLightState(0U);
    }
    else
    {
        APP_RELAY_SetLightState(1U);
    }
}

uint8_t APP_RELAY_GetLightState(void)
{
    return g_light_on_state;
}
