#include "main.h"
#include "usart.h"
#include "gpio.h"

#include <stdio.h>
#include <string.h>

#include "bsp_UART.h"
#include "bsp_OLED.h"
#include "dht11.h"
#include "stm32f4xx_hal_tim.h"

void SystemClock_Config(void);

static void PIR_Init(void);
static uint8_t PIR_IsDetected(void);
static void MQ135_Init(void);
static uint8_t MQ135_IsPolluted(void);
static void Servo_Init(void);
static void Servo_SetAngle(uint32_t channel, uint8_t angle);
static void Door_SetState(uint8_t open);
static void App_ShowDataOnOLED(uint8_t temp, uint8_t humi, uint8_t ok, uint8_t mqPolluted);

/* 人体红外模块引脚（可按接线修改） */
#define PIR_GPIO_PORT GPIOA
#define PIR_GPIO_PIN  GPIO_PIN_1
#define PIR_ACTIVE_LEVEL GPIO_PIN_SET

/* MQ135 (DO digital output) */
#define MQ135_GPIO_PORT GPIOC
#define MQ135_GPIO_PIN  GPIO_PIN_4
#define MQ135_POLLUTED_LEVEL GPIO_PIN_RESET

/* Key input: PA0 rising edge as press event */
#define KEY0_GPIO_PORT GPIOA
#define KEY0_GPIO_PIN  GPIO_PIN_0

/* Servo PWM output: TIM2 CH3(PA2), CH4(PA3), 50Hz */
#define SERVO_TIMER TIM2
#define SERVO_CH_LEFT  TIM_CHANNEL_3
#define SERVO_CH_RIGHT TIM_CHANNEL_4

/* Door angle config (0~90) */
#define DOOR_ANGLE_CLOSE 0
#define DOOR_ANGLE_OPEN  90

static TIM_HandleTypeDef htim2;
static volatile uint8_t g_key0_irq_event = 0;

int main(void)
{
    uint8_t temp = 0;
    uint8_t humi = 0;
    uint8_t dhtInitOk = 0;
    uint8_t lastPir = 0xFF;
    uint8_t doorOpen = 0;

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();

    UART1_Init(115200);
    OLED_Init();
    OLED_Clear();
    PIR_Init();
    MQ135_Init();
    Servo_Init();
    Door_SetState(0);

    if (DHT11_Init() == 0)
    {
        dhtInitOk = 1;
        printf("DHT11 init ok\r\n");
    }
    else
    {
        printf("DHT11 init fail\r\n");
    }

    while (1)
    {
        uint8_t readOk = 0;
        uint8_t pirDetected = PIR_IsDetected();
        uint8_t mqPolluted = MQ135_IsPolluted();

        if (g_key0_irq_event)
        {
            g_key0_irq_event = 0;
            doorOpen = (uint8_t)!doorOpen;
            Door_SetState(doorOpen);
            printf("DOOR=%s\r\n", doorOpen ? "OPEN" : "CLOSE");
        }

        if (dhtInitOk && (DHT11_Read_Data(&temp, &humi) == 0))
        {
            readOk = 1;
            printf("TEMP=%uC HUMI=%u%% AQ=%s PIR=%s\r\n",
                   temp, humi, mqPolluted ? "BAD" : "GOOD", pirDetected ? "DETECTED" : "IDLE");
        }
        else
        {
            printf("DHT11 read fail AQ=%s PIR=%s\r\n",
                   mqPolluted ? "BAD" : "GOOD", pirDetected ? "DETECTED" : "IDLE");
        }

        /* PC5 低电平点亮：检测到人体时亮灯 */
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, pirDetected ? GPIO_PIN_RESET : GPIO_PIN_SET);

        /* PIR状态变化时补充提示 */
        if (pirDetected != lastPir)
        {
            lastPir = pirDetected;
            printf("PIR %s\r\n", pirDetected ? "DETECTED" : "IDLE");
        }

        App_ShowDataOnOLED(temp, humi, readOk, mqPolluted);
        HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
        HAL_Delay(1000);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

static void PIR_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (PIR_GPIO_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (PIR_GPIO_PORT == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitStruct.Pin = PIR_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PIR_GPIO_PORT, &GPIO_InitStruct);
}

static uint8_t PIR_IsDetected(void)
{
    return (HAL_GPIO_ReadPin(PIR_GPIO_PORT, PIR_GPIO_PIN) == PIR_ACTIVE_LEVEL) ? 1U : 0U;
}

static void MQ135_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (MQ135_GPIO_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (MQ135_GPIO_PORT == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitStruct.Pin = MQ135_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MQ135_GPIO_PORT, &GPIO_InitStruct);
}

static uint8_t MQ135_IsPolluted(void)
{
    return (HAL_GPIO_ReadPin(MQ135_GPIO_PORT, MQ135_GPIO_PIN) == MQ135_POLLUTED_LEVEL) ? 1U : 0U;
}

static void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA2->TIM2_CH3, PA3->TIM2_CH4 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim2.Instance = SERVO_TIMER;
    htim2.Init.Prescaler = 84 - 1;    /* 84MHz/84 = 1MHz -> 1us */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 20000 - 1;    /* 20ms -> 50Hz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;            /* default: 0 deg */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO_CH_LEFT);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO_CH_RIGHT);

    HAL_TIM_PWM_Start(&htim2, SERVO_CH_LEFT);
    HAL_TIM_PWM_Start(&htim2, SERVO_CH_RIGHT);
}

static void Servo_SetAngle(uint32_t channel, uint8_t angle)
{
    uint32_t pulseUs;

    if (angle > 90) angle = 90;

    /* 0~90 deg -> 500~1500us */
    pulseUs = 500U + ((uint32_t)angle * 1000U) / 90U;
    __HAL_TIM_SET_COMPARE(&htim2, channel, pulseUs);
}

static void Door_SetState(uint8_t open)
{
    if (open)
    {
        /* Symmetric motion around cabinet center */
        Servo_SetAngle(SERVO_CH_LEFT, DOOR_ANGLE_OPEN);
        Servo_SetAngle(SERVO_CH_RIGHT, DOOR_ANGLE_CLOSE);
    }
    else
    {
        Servo_SetAngle(SERVO_CH_LEFT, DOOR_ANGLE_CLOSE);
        Servo_SetAngle(SERVO_CH_RIGHT, DOOR_ANGLE_OPEN);
    }
}

static void App_ShowDataOnOLED(uint8_t temp, uint8_t humi, uint8_t ok, uint8_t mqPolluted)
{
    static uint8_t inited = 0;
    static uint8_t lastTemp = 0xFF;
    static uint8_t lastHumi = 0xFF;
    static uint8_t lastOk = 0xFF;
    static uint8_t lastAQ = 0xFF;
    char line[20] = {0};

    if (!inited)
    {
        inited = 1;
        OLED_Clear();
        OLED_String(0, 0, "TEMP:", 16);
        OLED_String(0, 16, "HUMI:", 16);
        OLED_String(0, 32, "AQ:", 16);
    }

    if (ok)
    {
        if (lastOk != 1 || temp != lastTemp)
        {
            snprintf(line, sizeof(line), "%3uC   ", temp);
            OLED_String(48, 0, line, 16);
        }
        if (lastOk != 1 || humi != lastHumi)
        {
            snprintf(line, sizeof(line), "%3u%%   ", humi);
            OLED_String(48, 16, line, 16);
        }
    }
    else
    {
        if (lastOk != 0)
        {
            OLED_String(48, 0, "--C    ", 16);
            OLED_String(48, 16, "--%    ", 16);
        }
    }

    if (lastAQ != mqPolluted)
    {
        OLED_String(48, 32, mqPolluted ? "BAD   " : "GOOD  ", 16);
    }

    lastTemp = temp;
    lastHumi = humi;
    lastOk = ok;
    lastAQ = mqPolluted;
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY0_GPIO_PIN)
    {
        g_key0_irq_event = 1;
    }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
