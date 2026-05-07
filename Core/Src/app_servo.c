#include "app_servo.h"
#include "stm32f4xx_hal_tim.h"

/* 定时器句柄：仅在本文件内部使用 */
static TIM_HandleTypeDef g_app_servo_tim3;
static volatile uint8_t g_app_servo_ready = 0U;
static volatile uint8_t g_app_servo_pwm_started = 0U;

/**
  * @brief  启动TIM3双通道PWM输出
  * @param  无
  * @retval 1-成功, 0-失败
  */
static uint8_t APP_SERVO_StartOutput(void)
{
    HAL_StatusTypeDef hal_status;

    if (g_app_servo_pwm_started == 1U)
    {
        return 1U;
    }

    hal_status = HAL_TIM_PWM_Start(&g_app_servo_tim3, APP_SERVO_CH_LEFT);
    if (hal_status != HAL_OK)
    {
        return 0U;
    }

    hal_status = HAL_TIM_PWM_Start(&g_app_servo_tim3, APP_SERVO_CH_RIGHT);
    if (hal_status != HAL_OK)
    {
        (void)HAL_TIM_PWM_Stop(&g_app_servo_tim3, APP_SERVO_CH_LEFT);
        return 0U;
    }

    g_app_servo_pwm_started = 1U;
    return 1U;
}

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

    if (angle > 90U)
    {
        angle = 90U;
    }

    pulse_us = 500U + ((uint32_t)angle * 1000U) / 90U;
    __HAL_TIM_SET_COMPARE(&g_app_servo_tim3, tim_channel, pulse_us);
}

/**
  * @brief  舵机驱动初始化函数
  * @param  无
  * @retval 无
  * @note   初始化TIM3 PWM参数，不在此处立即启动输出，避免上电异动
  */
void APP_SERVO_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    TIM_OC_InitTypeDef tim_oc_config = {0};
    HAL_StatusTypeDef hal_status;

    g_app_servo_ready = 0U;
    g_app_servo_pwm_started = 0U;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    gpio_init_struct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    g_app_servo_tim3.Instance = APP_SERVO_TIMER;
    g_app_servo_tim3.Init.Prescaler = 84U - 1U;
    g_app_servo_tim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_app_servo_tim3.Init.Period = 20000U - 1U;
    g_app_servo_tim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_app_servo_tim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    hal_status = HAL_TIM_PWM_Init(&g_app_servo_tim3);
    if (hal_status != HAL_OK)
    {
        return;
    }

    tim_oc_config.OCMode = TIM_OCMODE_PWM1;
    tim_oc_config.Pulse = 500U;
    tim_oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    tim_oc_config.OCFastMode = TIM_OCFAST_DISABLE;
    hal_status = HAL_TIM_PWM_ConfigChannel(&g_app_servo_tim3, &tim_oc_config, APP_SERVO_CH_LEFT);
    if (hal_status != HAL_OK)
    {
        return;
    }

    hal_status = HAL_TIM_PWM_ConfigChannel(&g_app_servo_tim3, &tim_oc_config, APP_SERVO_CH_RIGHT);
    if (hal_status != HAL_OK)
    {
        return;
    }

    /* 先设置默认比较值，但不启动输出 */
    APP_SERVO_SetSingleAngle(APP_SERVO_CH_LEFT, APP_SERVO_DOOR_ANGLE_CLOSE);
    APP_SERVO_SetSingleAngle(APP_SERVO_CH_RIGHT, APP_SERVO_DOOR_ANGLE_OPEN);

    g_app_servo_ready = 1U;
}

/**
  * @brief  设置柜门状态
  * @param  open_state: 0-关门, 1-开门
  * @retval 无
  * @note   双舵机镜像动作，实现柜门对称开合
  */
void APP_SERVO_SetDoorState(uint8_t open_state)
{
    if (g_app_servo_ready == 0U)
    {
        return;
    }

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

    (void)APP_SERVO_StartOutput();
}

/**
  * @brief  查询舵机PWM是否已就绪
  * @param  无
  * @retval 1-就绪, 0-未就绪
  */
uint8_t APP_SERVO_IsReady(void)
{
    return g_app_servo_ready;
}
