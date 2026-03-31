#ifndef __APP_SERVO_H
#define __APP_SERVO_H

#include "main.h"
#include <stdint.h>

/* 双舵机PWM输出资源：TIM2_CH3(PA2), TIM2_CH4(PA3) */
#define APP_SERVO_TIMER      TIM2
#define APP_SERVO_CH_LEFT    TIM_CHANNEL_3
#define APP_SERVO_CH_RIGHT   TIM_CHANNEL_4

/* 柜门角度限制 */
#define APP_SERVO_DOOR_ANGLE_CLOSE 0U
#define APP_SERVO_DOOR_ANGLE_OPEN  90U

/**
  * @brief  舵机驱动初始化函数
  * @param  无
  * @retval 无
  * @note   初始化TIM2 PWM，频率50Hz
  */
void APP_SERVO_Init(void);

/**
  * @brief  设置柜门状态
  * @param  open_state: 0-关门, 1-开门
  * @retval 无
  * @note   双舵机镜像动作，实现柜门对称开合
  */
void APP_SERVO_SetDoorState(uint8_t open_state);

#endif
