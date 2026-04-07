---
name: stm32-hal-style
description: Write and refactor STM32 HAL C firmware with the same style as this repository. Use when implementing or modifying Core/Bsp modules, main-loop task scheduling, ISR-to-flag event flow, ESP8266 AT command handling, and README hardware/protocol documentation.
---

# STM32 HAL Style

Follow these rules to keep code style and architecture consistent.

## 1) Keep the same architecture

- Keep `main.c` as scheduler center, not hardware-detail center.
- Keep ISR minimal: only set flags or call lightweight callback.
- Keep hardware operations in dedicated modules (`app_*` or `bsp_*`).
- Use periodic tasks via `HAL_GetTick()` and short loop delay.

## 2) Naming conventions

- Public app APIs: `APP_<MODULE>_<Action>()`
- BSP APIs: keep existing style (`UARTx_...`, `DHT11_...`, `OLED_...`)
- Global/static flags:
  - `static volatile uint8_t g_xxx = 0U;`
  - Prefer semantic suffixes: `_state`, `_ok`, `_online`, `_value`, `_tick`
- Macros are all-uppercase with clear unit meaning:
  - `*_PERIOD_MS`, `*_BUF_LEN`, `*_MAX_*`

## 3) Function organization

- Suggested order in `.c` file:
  1. includes
  2. file header comment block
  3. static function declarations
  4. static/global state
  5. public functions
  6. static helper functions
- Keep single responsibility per function.
- Prefer guard + early return for invalid/not-ready cases.

## 4) C coding details

- Use fixed-width integer types: `uint8_t/uint16_t/uint32_t`.
- Use unsigned suffix `U` for constants: `0U`, `1U`, `500U`.
- For intentionally ignored return values, write `(void)func(...);`.
- Keep boundary checks explicit for buffers, link IDs, payload length.

## 5) Comments and docs

- Keep concise engineering comments.
- Use Doxygen-style blocks for key functions:
  - `@brief`, `@param`, `@retval`, `@note`
- At file top, state design constraints (for example ISR rules/task rules).

## 6) Event and protocol style

- Input first -> pending command/flag -> execute in main context.
- Never execute heavy actuator action directly in interrupt context.
- For text protocol parsing:
  - trim CR/LF/spaces
  - normalize case if command is case-insensitive
  - ack unknown command with clear error hint
- Keep status upload in stable machine-readable format (JSON line).

## 7) README synchronization

If code behavior changes, update README in the same change:

- Function Summary section
- Pin Mapping section
- Code Structure section
- ESP8266 Interface section
- Design Constraints section

Pin docs should include direction and peer mapping when applicable (for example TX->RX).

## 8) Delivery checklist

- ISR has no blocking logic.
- Main loop does scheduling, not mixed low-level complexity.
- New APIs and flags follow naming pattern.
- Types/constant suffixes are consistent.
- Comments explain intent and boundaries, not trivial syntax.
- README is synced with pin/protocol/behavior changes.

---

# 中文版（翻译）

按以下规则保持与本仓库一致的代码风格与架构。

## 1）保持相同架构

- 让 `main.c` 作为调度中心，而不是硬件细节实现中心。
- ISR 保持最小化：只置位标志或调用轻量回调。
- 硬件操作放在独立模块（`app_*` 或 `bsp_*`）中。
- 通过 `HAL_GetTick()` 做周期任务，主循环保留短延时。

## 2）命名约定

- 应用层公开 API：`APP_<MODULE>_<Action>()`
- BSP API：沿用现有风格（`UARTx_...`、`DHT11_...`、`OLED_...`）
- 全局/静态状态变量：
  - `static volatile uint8_t g_xxx = 0U;`
  - 优先使用语义后缀：`_state`、`_ok`、`_online`、`_value`、`_tick`
- 宏名全部大写，且体现单位含义：
  - `*_PERIOD_MS`、`*_BUF_LEN`、`*_MAX_*`

## 3）函数组织

- `.c` 文件推荐顺序：
  1. includes
  2. 文件头注释块
  3. static 函数声明
  4. static/global 状态
  5. 公共函数
  6. static 辅助函数
- 单一函数尽量只做单一职责。
- 对无效参数/未就绪状态优先使用 guard + early return。

## 4）C 编码细节

- 使用定宽整型：`uint8_t/uint16_t/uint32_t`。
- 无符号常量使用 `U` 后缀：`0U`、`1U`、`500U`。
- 对有意忽略返回值的调用，写成 `(void)func(...);`。
- 缓冲区、链路 ID、负载长度等边界检查必须显式。

## 5）注释与文档

- 注释保持工程化、简洁、面向意图。
- 关键函数使用 Doxygen 风格块注释：
  - `@brief`、`@param`、`@retval`、`@note`
- 文件顶部写清设计约束（例如 ISR 规则、任务调度规则）。

## 6）事件与协议风格

- 输入先转换为待处理命令/标志，再在主循环上下文执行动作。
- 不在中断上下文直接执行重硬件动作。
- 文本协议解析要求：
  - 去除 CR/LF/空格
  - 对大小写不敏感命令先归一化
  - 未知命令返回清晰的错误提示
- 状态上报保持稳定、可机读（推荐 JSON 单行）。

## 7）README 同步要求

当代码行为变化时，同步更新 README：

- 功能简介
- 引脚映射
- 代码结构
- ESP8266 接口说明
- 设计约束

引脚文档需要包含方向与对端映射（例如 TX->RX）。

## 8）交付前检查清单

- ISR 内无阻塞逻辑。
- 主循环仅调度，不混入过多底层复杂操作。
- 新增 API 和状态变量遵守命名模式。
- 类型与常量后缀风格一致。
- 注释解释“意图与边界”，不是语法复述。
- 引脚/协议/行为变化已同步到 README。
