#include "main.h"
#include "usart.h"
#include "gpio.h"

#include <stdio.h>

#include "bsp_UART.h"
#include "bsp_OLED.h"
#include "dht11.h"
#include "bsp_Key.h"

#include "app_led.h"
#include "app_pir.h"
#include "app_servo.h"
#include "app_esp8266.h"

/******************************************************************************
 * 文件名称: main.c
 * 功能说明: 智能衣柜系统主业务文件
 *
 * 代码说明:
 * 1. 所有外设驱动均独立在对应模块文件中
 * 2. 主循环只做任务调度，不直接写复杂硬件操作
 * 3. 中断回调只做事件置位，不做延时和复杂逻辑
 ******************************************************************************/

void SystemClock_Config(void);

/**
  * @brief  应用层初始化函数
  * @param  无
  * @retval 无
  * @note   负责初始化业务所需全部外设模块
  */
static void APP_Init(void);

/**
  * @brief  处理按键事件任务
  * @param  无
  * @retval 无
  * @note   有按键事件时切换柜门状态
  */
static void APP_TaskProcessKeyEvent(void);

/**
  * @brief  采集传感器任务
  * @param  无
  * @retval 无
  * @note   周期读取人体红外状态与温湿度数据
  */
static void APP_TaskSampleSensors(void);

/**
  * @brief  更新执行器任务
  * @param  无
  * @retval 无
  * @note   根据当前状态更新红灯显示
  */
static void APP_TaskUpdateActuator(void);

/**
  * @brief  串口打印任务
  * @param  无
  * @retval 无
  * @note   将关键状态通过USART1输出
  */
static void APP_TaskReportUart(void);

/**
  * @brief  OLED刷新任务
  * @param  无
  * @retval 无
  * @note   使用局部刷新减少屏幕闪烁
  */
static void APP_TaskUpdateOLED(void);

/**
  * @brief  ESP8266命令处理任务
  * @param  无
  * @retval 无
  * @note   处理网络下发的开关门与状态查询命令
  */
static void APP_TaskProcessEspCommand(void);

/**
  * @brief  ESP8266上报任务
  * @param  无
  * @retval 无
  * @note   按周期将传感器/门状态上报到TCP客户端
  */
static void APP_TaskUploadEspStatus(void);

/* 全局状态标志：按规范使用volatile */
static volatile uint8_t g_door_open_state = 0U;   /* 柜门状态：0关门，1开门 */
static volatile uint8_t g_pir_detected = 0U;      /* 人体红外状态：1检测到人体 */
static volatile uint8_t g_dht_online = 0U;        /* DHT11在线标志：1初始化成功 */
static volatile uint8_t g_dht_ok = 0U;            /* DHT11本次读取状态：1成功 */
static volatile uint8_t g_temp_value = 0U;        /* 温度值 */
static volatile uint8_t g_humi_value = 0U;        /* 湿度值 */

int main(void)
{
    uint32_t last_sensor_tick = 0U;   /* 传感器采样节拍 */
    uint32_t last_heart_tick = 0U;    /* 心跳灯闪烁节拍 */

    HAL_Init();
    SystemClock_Config();

    /* CubeMX基础初始化 */
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* 业务模块初始化 */
    APP_Init();

    while (1)
    {
        /* 任务1：按键事件处理（本地按键） */
        APP_TaskProcessKeyEvent();

        /* 任务2：处理 ESP8266 串口接收（网络命令、连接状态） */
        APP_ESP8266_TaskProcessRx();

        /* 任务3：执行 ESP8266 下发命令 */
        APP_TaskProcessEspCommand();

        /* 任务4：执行器更新 */
        APP_TaskUpdateActuator();

        /* 任务5：按周期采样并刷新本地显示 */
        if ((HAL_GetTick() - last_sensor_tick) >= 2000U)
        {
            last_sensor_tick = HAL_GetTick();
            APP_TaskSampleSensors();
            APP_TaskReportUart();
            APP_TaskUpdateOLED();
        }

        /* 任务6：按周期上报数据到 ESP8266 TCP 客户端 */
        APP_TaskUploadEspStatus();

        /* 任务7：系统心跳灯翻转 */
        if ((HAL_GetTick() - last_heart_tick) >= 500U)
        {
            last_heart_tick = HAL_GetTick();
            APP_LED_ToggleHeartbeat();
        }

        /* 主循环快轮询，保持网络命令响应速度 */
        HAL_Delay(10);
    }
}

/**
  * @brief  应用层初始化函数
  * @param  无
  * @retval 无
  * @note   完成串口、OLED、按键、红外、舵机、DHT11、ESP8266初始化
  */
static void APP_Init(void)
{
    /* 串口驱动初始化：115200波特率 */
    UART1_Init(115200);

    /* OLED驱动初始化 */
    OLED_Init();
    OLED_Clear();

    /* LED驱动初始化 */
    APP_LED_Init();

    /* 按键驱动初始化 */
    Key_Init();

    /* 人体红外驱动初始化 */
    APP_PIR_Init();

    /* 舵机驱动初始化并默认关门 */
    APP_SERVO_Init();
    APP_SERVO_SetDoorState(0U);
    g_door_open_state = 0U;

    /* DHT11驱动初始化 */
    if (DHT11_Init() == 0U)
    {
        g_dht_online = 1U;
        printf("DHT11 init ok\r\n");
    }
    else
    {
        g_dht_online = 0U;
        printf("DHT11 init fail\r\n");
    }

    /* ESP8266 驱动初始化（UART2 + TCP多连接Server） */
    if (APP_ESP8266_Init() == 1U)
    {
        printf("ESP8266 init ok\r\n");
    }
    else
    {
        printf("ESP8266 init fail\r\n");
    }
}

/**
  * @brief  处理按键事件任务
  * @param  无
  * @retval 无
  * @note   每次按键事件切换一次柜门状态
  */
static void APP_TaskProcessKeyEvent(void)
{
    /* 读取并清除按键事件标志 */
    if (Key_GetEventAndClear() == 0U)
    {
        return;
    }

    /* 反转柜门状态 */
    g_door_open_state = (uint8_t)!g_door_open_state;

    /* 更新舵机门状态 */
    APP_SERVO_SetDoorState(g_door_open_state);

    /* 串口打印门状态 */
    printf("DOOR=%s\r\n", g_door_open_state ? "OPEN" : "CLOSE");
}

/**
  * @brief  ESP8266命令处理任务
  * @param  无
  * @retval 无
  * @note   网络命令只在主循环执行硬件动作，不在中断中执行
  */
static void APP_TaskProcessEspCommand(void)
{
    APP_ESP8266_CmdTypeDef cmd_type;
    uint8_t cmd_link_id;

    if (APP_ESP8266_GetPendingCommand(&cmd_type, &cmd_link_id) == 0U)
    {
        return;
    }

    if (cmd_type == APP_ESP8266_CMD_DOOR_OPEN)
    {
        g_door_open_state = 1U;
        APP_SERVO_SetDoorState(g_door_open_state);
        printf("ESP CMD(link %u): DOOR OPEN\r\n", cmd_link_id);
    }
    else if (cmd_type == APP_ESP8266_CMD_DOOR_CLOSE)
    {
        g_door_open_state = 0U;
        APP_SERVO_SetDoorState(g_door_open_state);
        printf("ESP CMD(link %u): DOOR CLOSE\r\n", cmd_link_id);
    }
    else if (cmd_type == APP_ESP8266_CMD_DOOR_TOGGLE)
    {
        g_door_open_state = (uint8_t)!g_door_open_state;
        APP_SERVO_SetDoorState(g_door_open_state);
        printf("ESP CMD(link %u): DOOR TOGGLE -> %s\r\n",
               cmd_link_id,
               g_door_open_state ? "OPEN" : "CLOSE");
    }
    else if (cmd_type == APP_ESP8266_CMD_STATUS_QUERY)
    {
        /* 收到状态查询命令后，立刻回发当前状态给对应连接 */
        (void)APP_ESP8266_SendStatusNow(cmd_link_id,
                                        g_dht_ok,
                                        g_temp_value,
                                        g_humi_value,
                                        g_pir_detected,
                                        g_door_open_state);
        printf("ESP CMD(link %u): STATUS QUERY\r\n", cmd_link_id);
    }
    else
    {
        /* 无效命令类型，忽略 */
    }
}

/**
  * @brief  采集传感器任务
  * @param  无
  * @retval 无
  * @note   读取PIR与DHT11
  */
static void APP_TaskSampleSensors(void)
{
    /* 读取人体红外状态 */
    g_pir_detected = APP_PIR_ReadState();

    /* 读取温湿度数据 */
    if (g_dht_online == 1U)
    {
        if (DHT11_Read_Data((uint8_t *)&g_temp_value, (uint8_t *)&g_humi_value) == 0U)
        {
            g_dht_ok = 1U;
        }
        else
        {
            g_dht_ok = 0U;
        }
    }
    else
    {
        g_dht_ok = 0U;
    }
}

/**
  * @brief  更新执行器任务
  * @param  无
  * @retval 无
  * @note   根据人体红外状态控制红灯
  */
static void APP_TaskUpdateActuator(void)
{
    APP_LED_SetPIRIndicator(g_pir_detected);
}

/**
  * @brief  串口打印任务
  * @param  无
  * @retval 无
  * @note   打印温湿度与人体红外状态
  */
static void APP_TaskReportUart(void)
{
    static uint8_t last_pir_state = 0xFFU;  /* 记录上一次PIR状态，仅变化时提示 */

    if (g_dht_ok == 1U)
    {
        printf("TEMP=%uC HUMI=%u%% PIR=%s\r\n",
               g_temp_value,
               g_humi_value,
               g_pir_detected ? "DETECTED" : "IDLE");
    }
    else
    {
        printf("DHT11 read fail PIR=%s\r\n",
               g_pir_detected ? "DETECTED" : "IDLE");
    }

    /* PIR状态变化时额外打印 */
    if (last_pir_state != g_pir_detected)
    {
        last_pir_state = g_pir_detected;
        printf("PIR %s\r\n", g_pir_detected ? "DETECTED" : "IDLE");
    }
}

/**
  * @brief  OLED刷新任务
  * @param  无
  * @retval 无
  * @note   仅刷新变化区域，降低闪烁
  */
static void APP_TaskUpdateOLED(void)
{
    static uint8_t oled_inited = 0U;         /* OLED模板是否已绘制 */
    static uint8_t last_temp = 0xFFU;        /* 上次温度值 */
    static uint8_t last_humi = 0xFFU;        /* 上次湿度值 */
    static uint8_t last_dht_ok = 0xFFU;      /* 上次DHT读取状态 */
    static uint8_t last_pir = 0xFFU;         /* 上次PIR状态 */
    char text_buffer[20] = {0};              /* 显示字符串缓冲区 */

    /* 首次进入时绘制固定标签 */
    if (oled_inited == 0U)
    {
        oled_inited = 1U;
        OLED_Clear();
        OLED_String(0, 0,  "TEMP:", 16);
        OLED_String(0, 16, "HUMI:", 16);
        OLED_String(0, 32, "PIR:", 16);
    }

    /* 温湿度显示刷新 */
    if (g_dht_ok == 1U)
    {
        if ((last_dht_ok != 1U) || (last_temp != g_temp_value))
        {
            snprintf(text_buffer, sizeof(text_buffer), "%3uC   ", g_temp_value);
            OLED_String(48, 0, text_buffer, 16);
        }

        if ((last_dht_ok != 1U) || (last_humi != g_humi_value))
        {
            snprintf(text_buffer, sizeof(text_buffer), "%3u%%   ", g_humi_value);
            OLED_String(48, 16, text_buffer, 16);
        }
    }
    else
    {
        if (last_dht_ok != 0U)
        {
            OLED_String(48, 0,  "--C    ", 16);
            OLED_String(48, 16, "--%    ", 16);
        }
    }

    /* PIR显示刷新 */
    if (last_pir != g_pir_detected)
    {
        OLED_String(48, 32, g_pir_detected ? "DETECTED " : "IDLE     ", 16);
    }

    /* 保存本次状态，用于下次比较 */
    last_temp = g_temp_value;
    last_humi = g_humi_value;
    last_dht_ok = g_dht_ok;
    last_pir = g_pir_detected;
}

/**
  * @brief  ESP8266上报任务
  * @param  无
  * @retval 无
  * @note   上报周期在模块内部控制，本函数仅调用接口
  */
static void APP_TaskUploadEspStatus(void)
{
    APP_ESP8266_TaskUploadStatus(g_dht_ok,
                                 g_temp_value,
                                 g_humi_value,
                                 g_pir_detected,
                                 g_door_open_state);
}

/**
  * @brief  外部中断回调函数
  * @param  GPIO_Pin: 中断来源引脚
  * @retval 无
  * @note   中断中仅调用按键驱动事件置位函数
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    Key_EXTI_Callback(GPIO_Pin);
}

/**
  * @brief  系统时钟配置函数
  * @param  无
  * @retval 无
  * @note   配置系统时钟为168MHz
  */
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

/**
  * @brief  错误处理函数
  * @param  无
  * @retval 无
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  assert失败回调函数
  * @param  file: 文件名
  * @param  line: 行号
  * @retval 无
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
