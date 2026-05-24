#include "app_esp8266.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "bsp_UART.h"

/******************************************************************************
 * 鏂囦欢鍚嶇О: app_esp8266.c
 * 鍔熻兘璇存槑: ESP8266 搴旂敤灞傛帴鍙ｅ疄鐜? *
 * 鍗忚绾﹀畾:
 * 1. 璁惧涓婃姤鏍煎紡锛堟枃鏈琂SON锛夛細
 *    {"temp":25,"humi":60,"pir":1,"door":0,"dht":1}\r\n
 * 2. 涓嬭鍛戒护锛堜笉鍖哄垎澶у皬鍐欙級锛? *    DOOR=OPEN
 *    DOOR=CLOSE
 *    DOOR=TOGGLE
 *    GET=STATUS
 ******************************************************************************/

/* ESP8266 鍒濆鍖栧畬鎴愭爣蹇楋細1-宸插畬鎴愬垵濮嬪寲锛?-鏈畬鎴?*/
static volatile uint8_t g_esp8266_ready = 0U;

/* 寰呭鐞嗙綉缁滃懡浠わ細鍦ㄨВ鏋愬埌 +IPD 鎸囦护鍚庣疆浣嶏紝涓诲惊鐜鍙栧悗娓呴浂 */
static volatile APP_ESP8266_CmdTypeDef g_pending_cmd = APP_ESP8266_CMD_NONE;

/* 寰呭鐞嗗懡浠ゆ潵婧愮殑杩炴帴鍙?*/
static volatile uint8_t g_pending_cmd_link_id = 0U;

/* 澶氳繛鎺ュ湪绾跨姸鎬佽〃锛氭暟缁勪笅鏍囧嵆杩炴帴鍙凤紙0~4锛?*/
static volatile uint8_t g_link_online[APP_ESP8266_MAX_LINK_NUM] = {0U};

/* 鍛ㄦ湡涓婃姤鏃堕棿鎴筹紙鍗曚綅锛氭绉掞級 */
static uint32_t g_last_upload_tick = 0U;

/**
  * @brief  绛夊緟涓插彛杩斿洖鎸囧畾搴旂瓟瀛楃涓?  * @param  ack_string: 鏈熸湜搴旂瓟瀛楃涓?  * @param  timeout_ms: 瓒呮椂鏃堕棿锛堟绉掞級
  * @retval 1-绛夊緟鎴愬姛锛?-瓒呮椂澶辫触
  */
static uint8_t APP_ESP8266_WaitAck(const char *ack_string, uint16_t timeout_ms);

/**
  * @brief  鍙戦€佷竴鏉?AT 鎸囦护骞舵鏌ヨ繑鍥?  * @param  at_cmd: AT 鎸囦护瀛楃涓诧紙闇€甯?\r\n锛?  * @param  ack_string: 鏈熸湜杩斿洖鍏抽敭瀛?  * @param  timeout_ms: 瓒呮椂鏃堕棿锛堟绉掞級
  * @retval 1-鎴愬姛锛?-澶辫触
  */
static uint8_t APP_ESP8266_SendATAndCheck(const char *at_cmd, const char *ack_string, uint16_t timeout_ms);

/**
  * @brief  瑙ｆ瀽杩炴帴寤虹珛/鏂紑浜嬩欢
  * @param  rx_text: 鎺ユ敹鍒扮殑涓€甯т覆鍙ｆ枃鏈?  * @retval 鏃?  */
static void APP_ESP8266_ParseLinkEvent(const char *rx_text);

/**
  * @brief  瑙ｆ瀽 +IPD 甯у苟鎻愬彇鍛戒护
  * @param  rx_text: 鎺ユ敹鍒扮殑涓€甯т覆鍙ｆ枃鏈?  * @retval 鏃?  */
static void APP_ESP8266_ParseIPDCommand(const char *rx_text);

/**
  * @brief  鍚戞寚瀹氳繛鎺ュ彂閫佹枃鏈暟鎹?  * @param  link_id: 鐩爣杩炴帴鍙?  * @param  text: 鍙戦€佹枃鏈?  * @retval 1-鎴愬姛锛?-澶辫触
  */
static uint8_t APP_ESP8266_SendTextToLink(uint8_t link_id, const char *text);

/**
  * @brief  缁勭粐鐘舵€丣SON瀛楃涓?  * @param  out_text: 杈撳嚭瀛楃涓茬紦瀛?  * @param  out_len: 缂撳瓨闀垮害
  * @param  dht_ok: DHT11 鏁版嵁鏈夋晥鏍囧織
  * @param  temp: 娓╁害鍊?  * @param  humi: 婀垮害鍊?  * @param  pir: 浜轰綋绾㈠鐘舵€?  * @param  door_open: 鏌滈棬鐘舵€?  * @retval 鏃?  */
static void APP_ESP8266_FormatStatus(char *out_text,
                                     uint16_t out_len,
                                     uint8_t dht_ok,
                                     uint8_t temp,
                                     uint8_t humi,
                                     uint8_t pir,
                                     uint8_t door_open,
                                     uint8_t light_on);

/**
  * @brief  灏嗗瓧绗︿覆杞崲涓哄ぇ鍐欙紙渚夸簬涓嶅尯鍒嗗ぇ灏忓啓鍖归厤鍛戒护锛?  * @param  text: 寰呰浆鎹㈠瓧绗︿覆
  * @retval 鏃?  */
static void APP_ESP8266_ToUpper(char *text);

uint8_t APP_ESP8266_Init(void)
{
    uint8_t retry_count;
    char at_cmd[32] = {0};

    /* 鍏堟竻闆跺唴閮ㄧ姸鎬佸彉閲忥紝纭繚閲嶅鍒濆鍖栨椂鐘舵€佸共鍑€ */
    g_esp8266_ready = 0U;
    g_pending_cmd = APP_ESP8266_CMD_NONE;
    g_pending_cmd_link_id = 0U;
    memset((void *)g_link_online, 0, sizeof(g_link_online));

    /* 鍒濆鍖?UART2锛圥A2/PA3锛夛紝涓?ESP8266 涓插彛閫氫俊 */
    UART2_Init(APP_ESP8266_UART_BAUDRATE);

    /* 涓哄吋瀹逛笂鐢垫椂闂村樊锛孉T 鍛戒护鍏堝仛鏈€澶?娆￠噸璇?*/
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

    /* 鍏抽棴 AT 鍥炴樉锛屽噺灏戜覆鍙ｅ櫔澹帮紝渚夸簬瑙ｆ瀽 */
    (void)APP_ESP8266_SendATAndCheck("ATE0\r\n", "OK", 500U);

    /* 璁剧疆 Wi-Fi 妯″紡锛? = softAP + station锛堜笌鏂囨。绀轰緥淇濇寔涓€鑷达級 */
    if (APP_ESP8266_SendATAndCheck("AT+CWMODE=3\r\n", "OK", 1000U) == 0U)
    {
        printf("ESP8266 init fail: CWMODE\r\n");
        return 0U;
    }

    /* 浣胯兘澶氳繛鎺ワ紙TCP Server 蹇呴』鍏堝紑鍚杩炴帴锛?*/
    if (APP_ESP8266_SendATAndCheck("AT+CIPMUX=1\r\n", "OK", 1000U) == 0U)
    {
        printf("ESP8266 init fail: CIPMUX\r\n");
        return 0U;
    }

    /* 鍏堝皾璇曞叧闂巻鍙?server锛堥伩鍏嶉噸鍚悗閲嶅寮€鍚鑷?ERROR锛?*/
    (void)APP_ESP8266_SendATAndCheck("AT+CIPSERVER=0\r\n", "OK", 500U);

    /* 寮€鍚?TCP Server锛堝杩炴帴锛?*/
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

    /* 鏈畬鎴愬垵濮嬪寲鏃讹紝涓嶅鐞嗕覆鍙ｆ暟鎹?*/
    if (g_esp8266_ready == 0U)
    {
        return;
    }

    /* 鏃犳柊鏁版嵁鍒欑洿鎺ヨ繑鍥?*/
    if (UART2_GetRxNum() == 0U)
    {
        return;
    }

    /* 鑾峰彇涓€甯у畬鏁存帴鏀舵枃鏈紙鐢?UART2 绌洪棽涓柇灏佸寘锛?*/
    rx_text = (const char *)UART2_GetRxData();
    if (rx_text == NULL)
    {
        UART2_ClearRx();
        return;
    }

    /* 鍏堣В鏋愯繛鎺ヤ笂涓嬬嚎浜嬩欢 */
    APP_ESP8266_ParseLinkEvent(rx_text);

    /* 鍐嶈В鏋?+IPD 涓嬭鍛戒护甯?*/
    APP_ESP8266_ParseIPDCommand(rx_text);

    /* 娓呴櫎鎺ユ敹鏍囧織锛岃繘鍏ヤ笅涓€杞帴鏀?*/
    UART2_ClearRx();
}

void APP_ESP8266_TaskUploadStatus(uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open,
                                  uint8_t light_on)
{
    uint32_t now_tick;
    uint8_t link_id;
    char status_text[96] = {0};

    /* 妯″潡鏈氨缁紝涓嶆墽琛屼笂鎶?*/
    if (g_esp8266_ready == 0U)
    {
        return;
    }

    /* 鍩轰簬鏃堕棿鎴宠繘琛屽懆鏈熸帶鍒?*/
    now_tick = HAL_GetTick();
    if ((now_tick - g_last_upload_tick) < APP_ESP8266_UPLOAD_PERIOD_MS)
    {
        return;
    }
    g_last_upload_tick = now_tick;

    /* 鍏堢粍缁囨湰娆′笂鎶ユ暟鎹枃鏈?*/
    APP_ESP8266_FormatStatus(status_text, sizeof(status_text), dht_ok, temp, humi, pir, door_open, light_on);

    /* 骞挎挱缁欐墍鏈夊湪绾胯繛鎺?*/
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

    /* 涓诲惊鐜鍙栧悗绔嬪嵆娓呯┖锛岄伩鍏嶉噸澶嶆墽琛?*/
    g_pending_cmd = APP_ESP8266_CMD_NONE;
    g_pending_cmd_link_id = 0U;

    return 1U;
}

uint8_t APP_ESP8266_SendStatusNow(uint8_t link_id,
                                  uint8_t dht_ok,
                                  uint8_t temp,
                                  uint8_t humi,
                                  uint8_t pir,
                                  uint8_t door_open,
                                  uint8_t light_on)
{
    char status_text[96] = {0};

    APP_ESP8266_FormatStatus(status_text, sizeof(status_text), dht_ok, temp, humi, pir, door_open, light_on);
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

            /* 鏈変簺杩斿洖甯ч噷浼氬す甯?CONNECT/CLOSED锛岃繖閲岄『渚跨淮鎶よ繛鎺ョ姸鎬?*/
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

    /* 濡傛灉 Wi-Fi 鏂紑锛屽垯鏈湴杩炴帴琛ㄦ竻闆?*/
    if (strstr(rx_text, "WIFI DISCONNECT") != NULL)
    {
        memset((void *)g_link_online, 0, sizeof(g_link_online));
        return;
    }

    /* 妫€鏌ユ瘡涓繛鎺ュ彿鐨?CONNECT/CLOSED 鏂囨湰 */
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

    /* 鏌ユ壘 +IPD 鏁版嵁甯уご */
    ipd_head = strstr(rx_text, "+IPD,");
    if (ipd_head == NULL)
    {
        return;
    }

    /* AT 杩斿洖鏍煎紡: +IPD,<link>,<len>:<payload> */
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

    /* 闃叉瓒婄晫锛氭嫹璐濋暱搴﹀彇 payload_len 涓庣紦瀛樹笂闄愮殑杈冨皬鍊?*/
    copy_len = (uint16_t)payload_len;
    if (copy_len >= APP_ESP8266_CMD_BUF_LEN)
    {
        copy_len = APP_ESP8266_CMD_BUF_LEN - 1U;
    }

    /* 鑻ュ疄闄呮敹鍒版暟鎹笉瓒冲０鏄庨暱搴︼紝鍒欐寜瀹為檯瀛楃涓查暱搴︽埅鏂?*/
    if (copy_len > (uint16_t)strlen(payload_start))
    {
        copy_len = (uint16_t)strlen(payload_start);
    }

    memcpy(cmd_text, payload_start, copy_len);
    cmd_text[copy_len] = '\0';

    /* 鍘绘帀灏鹃儴鎹㈣涓庣┖鏍硷紝閬垮厤鍖归厤澶辫触 */
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

    /* 鎸囦护杞ぇ鍐欙紝鍋氬埌涓嶅尯鍒嗗ぇ灏忓啓 */
    APP_ESP8266_ToUpper(cmd_text);

    /* 鏍规嵁鍗忚鏂囨湰鐢熸垚涓氬姟鍛戒护 */
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

    /* 鏈瘑鍒懡浠わ紝鍥炲彂甯姪鎻愮ず */
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

    /* 1) 鍛婄煡 ESP8266 鍗冲皢鍙戦€佺殑鐩爣杩炴帴鍙峰拰瀛楄妭鏁?*/
    snprintf(at_cmd, sizeof(at_cmd), "AT+CIPSEND=%u,%u\r\n", link_id, text_len);
    if (APP_ESP8266_SendATAndCheck(at_cmd, ">", 800U) == 0U)
    {
        return 0U;
    }

    /* 2) 鍙戝疄闄呮暟鎹?*/
    UART2_SendString("%s", text);

    /* 3) 绛夊緟鍙戦€佸畬鎴愬簲绛?*/
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
                                     uint8_t door_open,
                                     uint8_t light_on)
{
    if ((out_text == NULL) || (out_len == 0U))
    {
        return;
    }

    snprintf(out_text,
             out_len,
             "{\"temp\":%u,\"humi\":%u,\"pir\":%u,\"door\":%u,\"light\":%u,\"dht\":%u}\r\n",
             temp,
             humi,
             pir,
             door_open,
             light_on,
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

