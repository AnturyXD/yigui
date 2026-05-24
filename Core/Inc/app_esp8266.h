#ifndef __APP_ESP8266_H
#define __APP_ESP8266_H

#include <stdint.h>

#define APP_ESP8266_MAX_LINK_NUM        5U
#define APP_ESP8266_UART_BAUDRATE       115200U
#define APP_ESP8266_TCP_SERVER_PORT     333U
#define APP_ESP8266_UPLOAD_PERIOD_MS    2000U
#define APP_ESP8266_CMD_BUF_LEN         96U

typedef enum
{
    APP_ESP8266_CMD_NONE = 0,
    APP_ESP8266_CMD_DOOR_OPEN,
    APP_ESP8266_CMD_DOOR_CLOSE,
    APP_ESP8266_CMD_DOOR_TOGGLE,
    APP_ESP8266_CMD_STATUS_QUERY
} APP_ESP8266_CmdTypeDef;

uint8_t APP_ESP8266_Init(void);
void APP_ESP8266_TaskProcessRx(void);

void APP_ESP8266_TaskUploadStatus(uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open,
                                  uint8_t light_on);

uint8_t APP_ESP8266_GetPendingCommand(APP_ESP8266_CmdTypeDef *cmd, uint8_t *link_id);

uint8_t APP_ESP8266_SendStatusNow(uint8_t link_id,
                                  uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open,
                                  uint8_t light_on);

uint8_t APP_ESP8266_IsReady(void);

#endif
