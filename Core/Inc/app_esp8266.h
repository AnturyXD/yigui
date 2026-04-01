#ifndef __APP_ESP8266_H
#define __APP_ESP8266_H

#include <stdint.h>

/******************************************************************************
 * 文件名称: app_esp8266.h
 * 功能说明: ESP8266 应用层接口头文件（使用 UART2 + AT 指令）
 *
 * 设计说明:
 * 1. 严格使用 Bsp/UART/bsp_UART.c 已封装的 UART2 接口
 * 2. ESP8266 工作于多连接 TCP Server 模式（AT+CIPMUX=1）
 * 3. 本模块负责网络收发，不直接操作舵机/LED等业务外设
 ******************************************************************************/

/* ESP8266 连接数量上限（ESP8266 AT 多连接通常支持 0~4 共5路） */
#define APP_ESP8266_MAX_LINK_NUM        5U

/* ESP8266 UART2 通信波特率 */
#define APP_ESP8266_UART_BAUDRATE       115200U

/* TCP Server 端口号（可在调试工具中按此端口连接） */
#define APP_ESP8266_TCP_SERVER_PORT     333U

/* 周期上报间隔（毫秒） */
#define APP_ESP8266_UPLOAD_PERIOD_MS    2000U

/* 接收指令文本的最大缓存长度 */
#define APP_ESP8266_CMD_BUF_LEN         96U

/**
  * @brief  ESP8266 网络命令类型定义
  */
typedef enum
{
    APP_ESP8266_CMD_NONE = 0,          /* 无命令 */
    APP_ESP8266_CMD_DOOR_OPEN,         /* 开门命令 */
    APP_ESP8266_CMD_DOOR_CLOSE,        /* 关门命令 */
    APP_ESP8266_CMD_DOOR_TOGGLE,       /* 切换门状态命令 */
    APP_ESP8266_CMD_STATUS_QUERY       /* 状态查询命令 */
} APP_ESP8266_CmdTypeDef;

/**
  * @brief  ESP8266 初始化函数
  * @param  无
  * @retval 1-初始化成功，0-初始化失败
  * @note   初始化 UART2，并完成多连接 TCP Server 模式配置
  */
uint8_t APP_ESP8266_Init(void);

/**
  * @brief  ESP8266 接收处理任务
  * @param  无
  * @retval 无
  * @note   解析 CONNECT/CLOSED 事件与 +IPD 数据帧
  */
void APP_ESP8266_TaskProcessRx(void);

/**
  * @brief  周期上传状态到所有在线 TCP 连接
  * @param  dht_ok: DHT11 数据有效标志，1-有效，0-无效
  * @param  temp: 温度值
  * @param  humi: 湿度值
  * @param  pir: 人体红外状态，1-有人，0-无人
  * @param  door_open: 柜门状态，1-开门，0-关门
  * @retval 无
  * @note   内部带周期限制，未到周期时不会重复发送
  */
void APP_ESP8266_TaskUploadStatus(uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open);

/**
  * @brief  获取待处理的网络命令
  * @param  cmd: 输出参数，返回命令类型
  * @param  link_id: 输出参数，返回命令来源连接号
  * @retval 1-取到命令，0-当前无命令
  */
uint8_t APP_ESP8266_GetPendingCommand(APP_ESP8266_CmdTypeDef *cmd, uint8_t *link_id);

/**
  * @brief  向指定连接立即发送一帧状态数据
  * @param  link_id: 目标连接号
  * @param  dht_ok: DHT11 数据有效标志
  * @param  temp: 温度值
  * @param  humi: 湿度值
  * @param  pir: 人体红外状态
  * @param  door_open: 柜门状态
  * @retval 1-发送成功，0-发送失败
  */
uint8_t APP_ESP8266_SendStatusNow(uint8_t link_id,
                                  uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open);

/**
  * @brief  查询 ESP8266 当前是否初始化成功
  * @param  无
  * @retval 1-已就绪，0-未就绪
  */
uint8_t APP_ESP8266_IsReady(void);

#endif

