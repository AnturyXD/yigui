# 智能衣柜系统

基于 `STM32F407 + HAL` 的智能衣柜控制工程。

## 功能简介

- 温湿度采集：`DHT11`
- 人体红外检测：`PIR`
- 柜门控制：双舵机对称开关（最大 90 度）
- 显示输出：`0.96 OLED`（软件 I2C）
- 串口日志：`USART1 115200`
- 无线通信：`ESP8266`（`UART2`，多连接 `TCP Server`）

## 主要引脚

- `PA0`：按键输入（EXTI中断）
- `PA1`：PIR 输入
- `PA5`：DHT11 数据线
- `PA2`：USART2_TX（连接 ESP8266 RXD）
- `PA3`：USART2_RX（连接 ESP8266 TXD）
- `PA6`：舵机1 PWM（TIM3_CH1）
- `PA7`：舵机2 PWM（TIM3_CH2）
- `PB8`：OLED SCL
- `PB9`：OLED SDA
- `PA9`：USART1_TX
- `PA10`：USART1_RX
- `PC5`：板载红灯（低电平点亮）
- `PB2`：板载蓝灯（心跳指示）

## 代码结构说明

- `Core/Src/main.c`：主循环状态调度、任务管理
- `Core/Src/app_esp8266.c`：ESP8266网络接口（AT、多连接Server、上传与命令解析）
- `Bsp/DHT11`：DHT11 驱动
- `Bsp/OLED_0.96_4P`：OLED 驱动
- `Bsp/UART`：串口重定向与收发封装

## ESP8266 网络接口

- 工作模式：`AT+CWMODE=3`（softAP + station）
- 多连接：`AT+CIPMUX=1`
- Server：`AT+CIPSERVER=1,333`
- 上报格式（JSON文本）：
  - `{"temp":25,"humi":60,"pir":1,"door":0,"dht":1}`
- 下行指令（不区分大小写）：
  - `DOOR=OPEN`
  - `DOOR=CLOSE`
  - `DOOR=TOGGLE`
  - `GET=STATUS`

## 设计约束

- 中断函数只置位标志，不做复杂处理
- 主循环只调度任务，不堆叠复杂硬件操作
- 所有硬件操作封装为独立函数，便于维护和移植
