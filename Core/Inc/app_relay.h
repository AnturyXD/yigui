#ifndef __APP_RELAY_H
#define __APP_RELAY_H

#include "main.h"
#include <stdint.h>

/* 继电器控制引脚: PB12，低电平触发 */
#define APP_RELAY_GPIO_PORT RELAY_LIGHT_GPIO_Port
#define APP_RELAY_GPIO_PIN  RELAY_LIGHT_Pin

void APP_RELAY_Init(void);
void APP_RELAY_SetLightState(uint8_t light_on);
void APP_RELAY_ToggleLight(void);
uint8_t APP_RELAY_GetLightState(void);

#endif
