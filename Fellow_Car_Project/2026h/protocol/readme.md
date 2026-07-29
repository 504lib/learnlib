# protocol

轻量串口帧协议库，适合嵌入式串口通信。内置环形缓冲队列，支持 ACK 超时重传、解析阶段看门狗，可选自定义队列。

## 功能特性

- **固定帧格式**：帧头 2B / 类型 1B / 长度 1B / 载荷 N 字节 / 校验 2B / 帧尾 2B
- **校验和**：覆盖 TYPE + LEN + PAYLOAD，16bit 求和校验
- **ACK / 超时重传**：按需开启，自动重传 + 超时回调
- **解析看门狗**：非 IDLE 状态超时自动复位，防止残缺帧串帧
- **内置队列**：开箱即用，无需外部提供 `Queue_Operations`
- **自定义队列**：通过 `isCustomQueue` 标志切换到用户队列
- **大端序序列化工具**：`uint32_t` / `int16_t` / `float` 的 BE 编解码

## 帧格式

```
[H1][H2][CMD][LEN][PAYLOAD...][CHK_H][CHK_L][T1][T2]
```

| 字段 | 偏移 | 大小 | 说明 |
|------|------|------|------|
| H1 / H2 | 0 | 2B | 帧头，可自定义 |
| CMD | 2 | 1B | 帧类型（0xFF 保留为 ACK 帧） |
| LEN | 3 | 1B | 载荷字节数 |
| PAYLOAD | 4 | N | 数据载荷 |
| CHK_H / CHK_L | 4+N | 2B | 校验和（大端），覆盖 CMD、LEN、PAYLOAD |
| T1 / T2 | 6+N | 2B | 帧尾，可自定义 |

最大载荷：`UART_PROTOCOL_FRAME_BUFFER_LEN - 8`（默认 8 字节）

## 快速开始

```c
#include "protocol.h"

/* 1. 声明协议实例 */
UART_protocol_t proto;

/* 2. 提供发送函数 */
bool my_tx(const uint8_t* data, uint16_t len) {
    return HAL_UART_Transmit(&huart, data, len, 100) == HAL_OK;
}

/* 3. 帧接收回调 */
void on_frame(uint8_t type, const uint8_t* payload, uint16_t len) {
    if (type == 0x10 && len >= 2)
        int16_t val = rd_i16_be(payload);
}

/* 4. 初始化（必选参数） */
Uart_Protocol_Init(&proto, &(Uart_Protocol_FunctionsParameters){
    .Head_Tial_Frame_struct = { .Headerframe1 = 0xAA, .Headerframe2 = 0x55,
                                .Tailframe1 = 0x55, .Tailframe2 = 0xAA },
    .transmit_function = my_tx,
    .frame_received_handler = on_frame,
});

/* 5. 按需注册 ACK（可选） */
Uart_Protocol_Register_ACK(&proto, &(Uart_Protocol_ACKFunctionsParameters){
    .GetTick = HAL_GetTick,
    .timeout_handler = my_timeout_handler,
});

/* 6. 按需注册解析看门狗（可选） */
Uart_Protocol_Register_Parse_WatchDog(&proto, HAL_GetTick, 200);

/* 7. UART 接收回调里喂数据 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *h, uint16_t size) {
    Uart_Protocol_ProcessReceivedDataBuffer(&proto, rx_buf, size);
}

/* 8. 主循环轮询 */
while (1) {
    Uart_Protocol_Loop(&proto);
}
```

## API 参考

### 核心

| 函数 | 说明 |
|------|------|
| `Uart_Protocol_Init(instance, required)` | 初始化协议实例（帧头尾、发送回调、接收回调） |
| `Uart_Protocol_Loop(instance)` | 主循环：看门狗检查 → ACK 超时检查 → 从队列取 1 字节解析 |

### 数据接收

| 函数 | 说明 |
|------|------|
| `Uart_Protocol_ProcessReceivedData8bit(instance, byte)` | 单字节入队 |
| `Uart_Protocol_ProcessReceivedDataBuffer(instance, data, len)` | 批量入队 |

### 数据发送

| 函数 | 说明 |
|------|------|
| `Uart_Protocol_Transmit_Frame(instance, payload, type, len)` | 组帧并发送。type=0xFF 保留为 ACK |

### 可选功能注册

| 函数 | 说明 |
|------|------|
| `Uart_Protocol_Register_ACK(instance, param)` | 开启 ACK 机制：收到数据帧自动回复 ACK，发送后等 ACK 超时重传 |
| `Uart_Protocol_Register_Parse_WatchDog(instance, get_tick, timeout_ms)` | 开启解析看门狗：同一解析状态超时自动复位 |
| `Uart_Protocol_Register_CustomQueue(instance, queue_ops)` | 替换内置队列为用户自定义队列 |

### 序列化工具

| 函数 | 说明 |
|------|------|
| `rd_u32_be(p)` / `wr_u32_be(p, v)` | uint32_t 大端 ↔ 本机 |
| `rd_i16_be(p)` / `wr_i16_be(p, v)` | int16_t 大端 ↔ 本机 |
| `rd_f32_be(p)` / `wr_f32_be(p, f)` | float 大端 ↔ 本机 |

## 自定义队列

默认使用内置环形队列（容量 = `UART_PROTOCOL_FRAME_BUFFER_LEN * 2`）。如需自定义：

```c
// 切换到自定义队列（替代内置队列）
Uart_Protocol_Register_CustomQueue(&proto, (Queue_Operations){
    .instance = &my_queue,
    .push = my_queue_push,
    .pop = my_queue_pop,
});
```

## 配置宏

在 `#include "protocol.h"` 前定义：

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `UART_PROTOCOL_FRAME_BUFFER_LEN` | 16 | 帧缓冲区大小（含帧头尾） |
| `STATIC_QUEUE_MAX_CAPACITY` | 64 | 内置队列最大容量 |
| `NO_LOG_ASSERT` | — | 定义后禁用断言 |

## 注意事项

- ACK 帧类型固定为 `0xFF`，用户帧请避免使用
- `Register_Parse_WatchDog` 超时值建议 100~400ms，太短可能误复位慢速 payload
- 内置队列容量 = `UART_PROTOCOL_FRAME_BUFFER_LEN * 2`，确保 `<= STATIC_QUEUE_MAX_CAPACITY`
- `Uart_Protocol_Loop` 每次只处理 1 字节，不要在中断里调它
- 接收回调中调用 `ProcessReceivedDataBuffer` 后，主循环 `Loop` 会自动消费

## 许可证

以仓库根目录许可证为准。
