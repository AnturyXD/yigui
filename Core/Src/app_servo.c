#include "app_servo.h"
#include "stm32f4xx_hal_tim.h"

/* 定时器句柄：仅在本文件内部使用 */
static TIM_HandleTypeDef g_app_servo_tim2;

/**
  * @brief  设置单个舵机角度
  * @param  tim_channel: PWM通道
  * @param  angle: 角度(0~90)
  * @retval 无
  * @note   0~90度映射为500~1500us
  */
static void APP_SERVO_SetSingleAngle(uint32_t tim_channel, uint8_t angle)
{
    uint32_t pulse_us;

    /* 角度限幅，防止越界 */
    if (angle > 90U)
    {
        angle = 90U;
    }

    /* 角度映射到脉宽 */
    pulse_us = 500U + ((uint32_t)angle * 1000U) / 90U;

    /* 更新比较值输出PWM */
    __HAL_TIM_SET_COMPARE(&g_app_servo_tim2, tim_channel, pulse_us);
}

/**
  * @brief  舵机驱动初始化函数
  * @param  无
  * @retval 无
  * @note   初始化TIM2 PWM，频率50Hz
  */
void APP_SERVO_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    TIM_OC_InitTypeDef tim_oc_config = {0};

    /* 使能GPIOA与TIM2时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* PA2/PA3复用为TIM2_CH3/CH4 */
    gpio_init_struct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* 配置TIM2基础参数：1MHz计数频率，20ms周期 */
    g_app_servo_tim2.Instance = APP_SERVO_TIMER;
    g_app_servo_tim2.Init.Prescaler = 84 - 1;
    g_app_servo_tim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_app_servo_tim2.Init.Period = 20000 - 1;
    g_app_servo_tim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_app_servo_tim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&g_app_servo_tim2);

    /* 配置PWM通道输出模式 */
    tim_oc_config.OCMode = TIM_OCMODE_PWM1;
    tim_oc_config.Pulse = 500;
    tim_oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    tim_oc_config.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_app_servo_tim2, &tim_oc_config, APP_SERVO_CH_LEFT);
    HAL_TIM_PWM_ConfigChannel(&g_app_servo_tim2, &tim_oc_config, APP_SERVO_CH_RIGHT);

    /* 启动两个PWM通道 */
    HAL_TIM_PWM_Start(&g_app_servo_tim2, APP_SERVO_CH_LEFT);
    HAL_TIM_PWM_Start(&g_app_servo_tim2, APP_SERVO_CH_RIGHT);
}

/**
  * @brief  设置柜门状态
  * @param  open_state: 0-关门, 1-开门
  * @retval 无
  * @note   双舵机镜像动作，实现柜门对称开合
  */
void APP_SERVO_SetDoorState(uint8_t open_state)
{
    if (open_state == 1U)
    {
        APP_SERVO_SetSingleAngle(APP_SERVO_CH_LEFT, APP_SERVO_DOOR_ANGLE_OPEN);
        APP_SERVO_SetSingleAngle(APP_SERVO_CH_RIGHT, APP_SERVO_DOOR_ANGLE_CLOSE);
    }
    else
    {
        APP_SERVO_SetSingleAngle(APP_SERVO_CH_LEFT, APP_SERVO_DOOR_ANGLE_CLOSE);
        APP_SERVO_SetSingleAngle(APP_SERVO_CH_RIGHT, APP_SERVO_DOOR_ANGLE_OPEN);
    }
}
