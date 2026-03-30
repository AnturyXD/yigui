#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DHT11_GPIO_PORT GPIOA
#define DHT11_GPIO_PIN  GPIO_PIN_5

uint8_t DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

#ifdef __cplusplus
}
#endif

#endif
