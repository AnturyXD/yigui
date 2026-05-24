# ESP8266 设备接入接口文档（App 端）

本文档面向手机 App 开发，说明如何通过 Wi-Fi + TCP 接入智能衣柜设备，并完成状态读取和门控。

实现依据：
- `Core/Src/app_esp8266.c`
- `Core/Inc/app_esp8266.h`
- `Core/Src/main.c`
- `README.md`

## 1. 总览

- 通信链路：`App <-> ESP8266(TCP Server) <-> STM32`
- 协议类型：`TCP`（明文文本协议，非 HTTP / MQTT）
- 服务器地址：`192.168.4.1:333`（设备 SoftAP 场景）
- 多连接：支持最多 `5` 路连接（`link_id=0~4`）

## 2. 连接流程

1. 手机连接设备热点。
2. App 建立 TCP Client 连接 `192.168.4.1:333`。
3. 连接成功后保持长连接，持续收包。
4. 发送命令，接收 ACK/ERR 和状态 JSON。

## 3. 报文规则

- 文本编码：UTF-8 / ASCII
- ACK、ERR、JSON 统一以 `\r\n` 结尾
- App 下发命令建议带 `\r\n`

## 4. 下行命令（App -> 设备）

命令不区分大小写。

- `DOOR=OPEN` -> `ACK:DOOR=OPEN\r\n`
- `DOOR=CLOSE` -> `ACK:DOOR=CLOSE\r\n`
- `DOOR=TOGGLE` -> `ACK:DOOR=TOGGLE\r\n`
- `GET=STATUS` -> `ACK:GET=STATUS\r\n`（随后立即返回一帧 JSON）

非法命令返回：

`ERR:CMD,USE DOOR=OPEN/CLOSE/TOGGLE OR GET=STATUS\r\n`

## 5. 上行状态（设备 -> App）

示例：

```json
{"temp":25,"humi":60,"pir":1,"door":0,"light":1,"dht":1}
```

字段定义：

- `temp`：温度（摄氏度）
- `humi`：湿度（百分比）
- `pir`：人体红外，`1=有人`，`0=无人`
- `door`：门状态，`1=开`，`0=关`
- `light`：灯状态，`1=开灯`，`0=关灯`
- `dht`：温湿度有效位，`1=有效`，`0=无效`

灯带控制说明：

- 灯带不接受网络下行开关命令。
- 灯带由人体红外自动控制：`pir=1` 自动开灯，`pir=0` 自动关灯。

发送时机：

- 周期广播：每约 2 秒
- 即时返回：收到 `GET=STATUS` 后对请求连接立即返回

## 6. App 端处理建议

- 按 `\r\n` 分帧，逐行解析。
- ACK 仅表示命令已接收，最终状态以 JSON 为准。
- 建议 ACK 超时 1~2 秒，超时重发 1 次。
- 断线后按退避重连（1s/2s/5s...）。
