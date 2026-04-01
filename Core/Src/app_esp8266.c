#include "app_esp8266.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "bsp_UART.h"

/******************************************************************************
 * 文件名称: app_esp8266.c
 * 功能说明: ESP8266 应用层接口实现
 *
 * 协议约定:
 * 1. 设备上报格式（文本JSON）：
 *    {"temp":25,"humi":60,"pir":1,"door":0,"dht":1}\r\n
 * 2. 下行命令（不区分大小写）：
 *    DOOR=OPEN
 *    DOOR=CLOSE
 *    DOOR=TOGGLE
 *    GET=STATUS
 ******************************************************************************/

/* ESP8266 初始化完成标志：1-已完成初始化，0-未完成 */
static volatile uint8_t g_esp8266_ready = 0U;

/* 待处理网络命令：在解析到 +IPD 指令后置位，主循环读取后清零 */
static volatile APP_ESP8266_CmdTypeDef g_pending_cmd = APP_ESP8266_CMD_NONE;

/* 待处理命令来源的连接号 */
static volatile uint8_t g_pending_cmd_link_id = 0U;

/* 多连接在线状态表：数组下标即连接号（0~4） */
static volatile uint8_t g_link_online[APP_ESP8266_MAX_LINK_NUM] = {0U};

/* 周期上报时间戳（单位：毫秒） */
static uint32_t g_last_upload_tick = 0U;

/**
  * @brief  等待串口返回指定应答字符串
  * @param  ack_string: 期望应答字符串
  * @param  timeout_ms: 超时时间（毫秒）
  * @retval 1-等待成功，0-超时失败
  */
static uint8_t APP_ESP8266_WaitAck(const char *ack_string, uint16_t timeout_ms);

/**
  * @brief  发送一条 AT 指令并检查返回
  * @param  at_cmd: AT 指令字符串（需带 \r\n）
  * @param  ack_string: 期望返回关键字
  * @param  timeout_ms: 超时时间（毫秒）
  * @retval 1-成功，0-失败
  */
static uint8_t APP_ESP8266_SendATAndCheck(const char *at_cmd, const char *ack_string, uint16_t timeout_ms);

/**
  * @brief  解析连接建立/断开事件
  * @param  rx_text: 接收到的一帧串口文本
  * @retval 无
  */
static void APP_ESP8266_ParseLinkEvent(const char *rx_text);

/**
  * @brief  解析 +IPD 帧并提取命令
  * @param  rx_text: 接收到的一帧串口文本
  * @retval 无
  */
static void APP_ESP8266_ParseIPDCommand(const char *rx_text);

/**
  * @brief  向指定连接发送文本数据
  * @param  link_id: 目标连接号
  * @param  text: 发送文本
  * @retval 1-成功，0-失败
  */
static uint8_t APP_ESP8266_SendTextToLink(uint8_t link_id, const char *text);

/**
  * @brief  组织状态JSON字符串
  * @param  out_text: 输出字符串缓存
  * @param  out_len: 缓存长度
  * @param  dht_ok: DHT11 数据有效标志
  * @param  temp: 温度值
  * @param  humi: 湿度值
  * @param  pir: 人体红外状态
  * @param  door_open: 柜门状态
  * @retval 无
  */
static void APP_ESP8266_FormatStatus(char *out_text,
                                     uint16_t out_len,
                                     uint8_t dht_ok,
                                     uint8_t temp,
                                     uint8_t humi,
                                     uint8_t pir,
                                     uint8_t door_open);

/**
  * @brief  将字符串转换为大写（便于不区分大小写匹配命令）
  * @param  text: 待转换字符串
  * @retval 无
  */
static void APP_ESP8266_ToUpper(char *text);

uint8_t APP_ESP8266_Init(void)
{
    uint8_t retry_count;
    char at_cmd[32] = {0};

    /* 先清零内部状态变量，确保重复初始化时状态干净 */
    g_esp8266_ready = 0U;
    g_pending_cmd = APP_ESP8266_CMD_NONE;
    g_pending_cmd_link_id = 0U;
    memset((void *)g_link_online, 0, sizeof(g_link_online));

    /* 初始化 UART2（PA2/PA3），与 ESP8266 串口通信 */
    UART2_Init(APP_ESP8266_UART_BAUDRATE);

    /* 为兼容上电时间差，AT 命令先做最多3次重试 */
    for (retry_count = 0U; retry_count < 3U; retry_count++)
    {
        if (APP_ESP8266_SendATAndCheck("AT\r\n", "OK", 800U) == 1U)
        {
            break;
        }
        HAL_Delay(200);
    }

    if (retry_count >= 3U)
    {
        printf("ESP8266 init fail: no AT response\r\n");
        return 0U;
    }

    /* 关闭 AT 回显，减少串口噪声，便于解析 */
    (void)APP_ESP8266_SendATAndCheck("ATE0\r\n", "OK", 500U);

    /* 设置 Wi-Fi 模式：3 = softAP + station（与文档示例保持一致） */
    if (APP_ESP8266_SendATAndCheck("AT+CWMODE=3\r\n", "OK", 1000U) == 0U)
    {
        printf("ESP8266 init fail: CWMODE\r\n");
        return 0U;
    }

    /* 使能多连接（TCP Server 必须先开启多连接） */
    if (APP_ESP8266_SendATAndCheck("AT+CIPMUX=1\r\n", "OK", 1000U) == 0U)
    {
        printf("ESP8266 init fail: CIPMUX\r\n");
        return 0U;
    }

    /* 先尝试关闭历史 server（避免重启后重复开启导致 ERROR） */
    (void)APP_ESP8266_SendATAndCheck("AT+CIPSERVER=0\r\n", "OK", 500U);

    /* 开启 TCP Server（多连接） */
    snprintf(at_cmd, sizeof(at_cmd), "AT+CIPSERVER=1,%u\r\n", APP_ESP8266_TCP_SERVER_PORT);
    if (APP_ESP8266_SendATAndCheck(at_cmd, "OK", 1500U) == 0U)
    {
        printf("ESP8266 init fail: CIPSERVER\r\n");
        return 0U;
    }

    g_esp8266_ready = 1U;
    g_last_upload_tick = HAL_GetTick();
    printf("ESP8266 server ready on TCP port %u\r\n", APP_ESP8266_TCP_SERVER_PORT);
    return 1U;
}

void APP_ESP8266_TaskProcessRx(void)
{
    const char *rx_text;

    /* 未完成初始化时，不处理串口数据 */
    if (g_esp8266_ready == 0U)
    {
        return;
    }

    /* 无新数据则直接返回 */
    if (UART2_GetRxNum() == 0U)
    {
        return;
    }

    /* 获取一帧完整接收文本（由 UART2 空闲中断封包） */
    rx_text = (const char *)UART2_GetRxData();
    if (rx_text == NULL)
    {
        UART2_ClearRx();
        return;
    }

    /* 先解析连接上下线事件 */
    APP_ESP8266_ParseLinkEvent(rx_text);

    /* 再解析 +IPD 下行命令帧 */
    APP_ESP8266_ParseIPDCommand(rx_text);

    /* 清除接收标志，进入下一轮接收 */
    UART2_ClearRx();
}

void APP_ESP8266_TaskUploadStatus(uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open)
{
    uint32_t now_tick;
    uint8_t link_id;
    char status_text[96] = {0};

    /* 模块未就绪，不执行上报 */
    if (g_esp8266_ready == 0U)
    {
        return;
    }

    /* 基于时间戳进行周期控制 */
    now_tick = HAL_GetTick();
    if ((now_tick - g_last_upload_tick) < APP_ESP8266_UPLOAD_PERIOD_MS)
    {
        return;
    }
    g_last_upload_tick = now_tick;

    /* 先组织本次上报数据文本 */
    APP_ESP8266_FormatStatus(status_text, sizeof(status_text), dht_ok, temp, humi, pir, door_open);

    /* 广播给所有在线连接 */
    for (link_id = 0U; link_id < APP_ESP8266_MAX_LINK_NUM; link_id++)
    {
        if (g_link_online[link_id] == 1U)
        {
            (void)APP_ESP8266_SendTextToLink(link_id, status_text);
        }
    }
}

uint8_t APP_ESP8266_GetPendingCommand(APP_ESP8266_CmdTypeDef *cmd, uint8_t *link_id)
{
    if ((cmd == NULL) || (link_id == NULL))
    {
        return 0U;
    }

    if (g_pending_cmd == APP_ESP8266_CMD_NONE)
    {
        return 0U;
    }

    *cmd = g_pending_cmd;
    *link_id = g_pending_cmd_link_id;

    /* 主循环读取后立即清空，避免重复执行 */
    g_pending_cmd = APP_ESP8266_CMD_NONE;
    g_pending_cmd_link_id = 0U;

    return 1U;
}

uint8_t APP_ESP8266_SendStatusNow(uint8_t link_id,
                                  uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open)
{
    char status_text[96] = {0};

    APP_ESP8266_FormatStatus(status_text, sizeof(status_text), dht_ok, temp, humi, pir, door_open);
    return APP_ESP8266_SendTextToLink(link_id, status_text);
}

uint8_t APP_ESP8266_IsReady(void)
{
    return g_esp8266_ready;
}

static uint8_t APP_ESP8266_WaitAck(const char *ack_string, uint16_t timeout_ms)
{
    const char *rx_text;

    while (timeout_ms > 0U)
    {
        if (UART2_GetRxNum() > 0U)
        {
            rx_text = (const char *)UART2_GetRxData();

            /* 有些返回帧里会夹带 CONNECT/CLOSED，这里顺便维护连接状态 */
            if (rx_text != NULL)
            {
                APP_ESP8266_ParseLinkEvent(rx_text);

                if (strstr(rx_text, ack_string) != NULL)
                {
                    UART2_ClearRx();
                    return 1U;
                }
            }

            UART2_ClearRx();
        }

        HAL_Delay(1);
        timeout_ms--;
    }

    return 0U;
}

static uint8_t APP_ESP8266_SendATAndCheck(const char *at_cmd, const char *ack_string, uint16_t timeout_ms)
{
    if ((at_cmd == NULL) || (ack_string == NULL))
    {
        return 0U;
    }

    return UART2_SendAT((char *)at_cmd, (char *)ack_string, timeout_ms);
}

static void APP_ESP8266_ParseLinkEvent(const char *rx_text)
{
    uint8_t link_id;
    char event_text[20] = {0};

    if (rx_text == NULL)
    {
        return;
    }

    /* 如果 Wi-Fi 断开，则本地连接表清零 */
    if (strstr(rx_text, "WIFI DISCONNECT") != NULL)
    {
        memset((void *)g_link_online, 0, sizeof(g_link_online));
        return;
    }

    /* 检查每个连接号的 CONNECT/CLOSED 文本 */
    for (link_id = 0U; link_id < APP_ESP8266_MAX_LINK_NUM; link_id++)
    {
        snprintf(event_text, sizeof(event_text), "%u,CONNECT", link_id);
        if (strstr(rx_text, event_text) != NULL)
        {
            g_link_online[link_id] = 1U;
            printf("ESP8266 link %u connected\r\n", link_id);
        }

        snprintf(event_text, sizeof(event_text), "%u,CLOSED", link_id);
        if (strstr(rx_text, event_text) != NULL)
        {
            g_link_online[link_id] = 0U;
            printf("ESP8266 link %u closed\r\n", link_id);
        }
    }
}

static void APP_ESP8266_ParseIPDCommand(const char *rx_text)
{
    const char *ipd_head;
    const char *payload_start;
    int link_id;
    int payload_len;
    uint16_t copy_len;
    uint16_t i;
    char cmd_text[APP_ESP8266_CMD_BUF_LEN] = {0};

    if (rx_text == NULL)
    {
        return;
    }

    /* 查找 +IPD 数据帧头 */
    ipd_head = strstr(rx_text, "+IPD,");
    if (ipd_head == NULL)
    {
        return;
    }

    /* AT 返回格式: +IPD,<link>,<len>:<payload> */
    payload_start = strchr(ipd_head, ':');
    if (payload_start == NULL)
    {
        return;
    }

    if (sscanf(ipd_head, "+IPD,%d,%d:", &link_id, &payload_len) != 2)
    {
        return;
    }

    if ((link_id < 0) || (link_id >= (int)APP_ESP8266_MAX_LINK_NUM))
    {
        return;
    }

    if (payload_len <= 0)
    {
        return;
    }

    payload_start++;

    /* 防止越界：拷贝长度取 payload_len 与缓存上限的较小值 */
    copy_len = (uint16_t)payload_len;
    if (copy_len >= APP_ESP8266_CMD_BUF_LEN)
    {
        copy_len = APP_ESP8266_CMD_BUF_LEN - 1U;
    }

    /* 若实际收到数据不足声明长度，则按实际字符串长度截断 */
    if (copy_len > (uint16_t)strlen(payload_start))
    {
        copy_len = (uint16_t)strlen(payload_start);
    }

    memcpy(cmd_text, payload_start, copy_len);
    cmd_text[copy_len] = '\0';

    /* 去掉尾部换行与空格，避免匹配失败 */
    for (i = copy_len; i > 0U; i--)
    {
        if ((cmd_text[i - 1U] == '\r') || (cmd_text[i - 1U] == '\n') || (cmd_text[i - 1U] == ' '))
        {
            cmd_text[i - 1U] = '\0';
        }
        else
        {
            break;
        }
    }

    /* 指令转大写，做到不区分大小写 */
    APP_ESP8266_ToUpper(cmd_text);

    /* 根据协议文本生成业务命令 */
    if (strstr(cmd_text, "DOOR=OPEN") != NULL)
    {
        g_pending_cmd = APP_ESP8266_CMD_DOOR_OPEN;
        g_pending_cmd_link_id = (uint8_t)link_id;
        (void)APP_ESP8266_SendTextToLink((uint8_t)link_id, "ACK:DOOR=OPEN\r\n");
        return;
    }

    if (strstr(cmd_text, "DOOR=CLOSE") != NULL)
    {
        g_pending_cmd = APP_ESP8266_CMD_DOOR_CLOSE;
        g_pending_cmd_link_id = (uint8_t)link_id;
        (void)APP_ESP8266_SendTextToLink((uint8_t)link_id, "ACK:DOOR=CLOSE\r\n");
        return;
    }

    if (strstr(cmd_text, "DOOR=TOGGLE") != NULL)
    {
        g_pending_cmd = APP_ESP8266_CMD_DOOR_TOGGLE;
        g_pending_cmd_link_id = (uint8_t)link_id;
        (void)APP_ESP8266_SendTextToLink((uint8_t)link_id, "ACK:DOOR=TOGGLE\r\n");
        return;
    }

    if (strstr(cmd_text, "GET=STATUS") != NULL)
    {
        g_pending_cmd = APP_ESP8266_CMD_STATUS_QUERY;
        g_pending_cmd_link_id = (uint8_t)link_id;
        (void)APP_ESP8266_SendTextToLink((uint8_t)link_id, "ACK:GET=STATUS\r\n");
        return;
    }

    /* 未识别命令，回发帮助提示 */
    (void)APP_ESP8266_SendTextToLink((uint8_t)link_id, "ERR:CMD,USE DOOR=OPEN/CLOSE/TOGGLE OR GET=STATUS\r\n");
}

static uint8_t APP_ESP8266_SendTextToLink(uint8_t link_id, const char *text)
{
    char at_cmd[32] = {0};
    uint16_t text_len;

    if (text == NULL)
    {
        return 0U;
    }

    if (link_id >= APP_ESP8266_MAX_LINK_NUM)
    {
        return 0U;
    }

    if (g_link_online[link_id] == 0U)
    {
        return 0U;
    }

    text_len = (uint16_t)strlen(text);
    if (text_len == 0U)
    {
        return 1U;
    }

    /* 1) 告知 ESP8266 即将发送的目标连接号和字节数 */
    snprintf(at_cmd, sizeof(at_cmd), "AT+CIPSEND=%u,%u\r\n", link_id, text_len);
    if (APP_ESP8266_SendATAndCheck(at_cmd, ">", 800U) == 0U)
    {
        return 0U;
    }

    /* 2) 发实际数据 */
    UART2_SendString("%s", text);

    /* 3) 等待发送完成应答 */
    if (APP_ESP8266_WaitAck("SEND OK", 1000U) == 0U)
    {
        return 0U;
    }

    return 1U;
}

static void APP_ESP8266_FormatStatus(char *out_text,
                                     uint16_t out_len,
                                     uint8_t dht_ok,
                                     uint8_t temp,
                                     uint8_t humi,
                                     uint8_t pir,
                                     uint8_t door_open)
{
    if ((out_text == NULL) || (out_len == 0U))
    {
        return;
    }

    snprintf(out_text,
             out_len,
             "{\"temp\":%u,\"humi\":%u,\"pir\":%u,\"door\":%u,\"dht\":%u}\r\n",
             temp,
             humi,
             pir,
             door_open,
             dht_ok);
}

static void APP_ESP8266_ToUpper(char *text)
{
    uint16_t index = 0U;

    if (text == NULL)
    {
        return;
    }

    while (text[index] != '\0')
    {
        text[index] = (char)toupper((unsigned char)text[index]);
        index++;
    }
}

