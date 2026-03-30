#include "dht11.h"

static void DHT11_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
    while ((DWT->CYCCNT - start) < ticks)
    {
    }
}

static void DHT11_SetPinOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_SetPinInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_Rst(void)
{
    DHT11_SetPinOutput();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    DHT11_DelayUs(30);
}

static uint8_t DHT11_Check(void)
{
    uint32_t retry = 0;

    DHT11_SetPinInput();

    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        DHT11_DelayUs(1);
        retry++;
        if (retry > 120)
        {
            return 1;
        }
    }

    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
    {
        DHT11_DelayUs(1);
        retry++;
        if (retry > 120)
        {
            return 1;
        }
    }

    return 0;
}

static uint8_t DHT11_Read_Bit(void)
{
    uint32_t retry = 0;

    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        DHT11_DelayUs(1);
        retry++;
        if (retry > 100)
        {
            return 0;
        }
    }

    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
    {
        DHT11_DelayUs(1);
        retry++;
        if (retry > 100)
        {
            return 0;
        }
    }

    DHT11_DelayUs(40);

    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        return 1;
    }

    return 0;
}

static uint8_t DHT11_Read_Byte(void)
{
    uint8_t i;
    uint8_t dat = 0;

    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }

    return dat;
}

uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t i;
    uint8_t buf[5] = {0};

    DHT11_Rst();
    if (DHT11_Check() != 0)
    {
        return 1;
    }

    for (i = 0; i < 5; i++)
    {
        buf[i] = DHT11_Read_Byte();
    }

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
    {
        return 1;
    }

    *humi = buf[0];
    *temp = buf[2];
    return 0;
}

uint8_t DHT11_Init(void)
{
    if (DHT11_GPIO_PORT == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOF)
    {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
    else if (DHT11_GPIO_PORT == GPIOG)
    {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    }

    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    DHT11_Rst();
    return DHT11_Check();
}
