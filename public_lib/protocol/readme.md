# protocol

一个轻量的串口帧协议模块，提供帧打包发送、帧解析校验、命令处理器注册与简单环形缓冲区，适合嵌入式串口通信。

## 功能特性
- 固定帧格式：帧头/类型/长度/载荷/校验/帧尾。
- 校验和：对 CMD + LEN + PAYLOAD 求和校验。
- 命令处理器注册：按命令类型绑定回调与参数长度。
- 发送端打包：自动拼帧并调用发送回调。
- 简单环形缓冲区：便于接收侧缓存原始字节流。
- 大端序工具：提供 uint32_t/float 的 BE 编解码。

## 帧格式
```
[H1][H2][CMD][LEN][PAYLOAD...][CHK_H][CHK_L][T1][T2]
```
- LEN 为 PAYLOAD 字节数。
- 校验覆盖 CMD、LEN 与 PAYLOAD。

## 主要文件
- protocol.h：协议结构体、命令类型、API 声明。
- protocol.c：协议实现与校验/环形缓冲区逻辑。

## 快速开始
1. 初始化 UART_Protocol_Init，传入帧头帧尾与发送函数。
2. 使用 UART_Protocol_Register_Hander 注册命令处理回调与参数长度。
3. 发送时组装 prama_Cmd_packet 并调用 UART_Protocol_Transmit。
4. 接收侧解析完整帧后调用 Receive_Uart_Frame。

## API 概览
- UART_Protocol_Init：初始化协议上下文。
- UART_Protocol_Register_Hander：注册命令处理器。
- UART_Protocol_Transmit：发送打包帧。
- Receive_Uart_Frame：解析并分发接收帧。
- Get_Uart_Frame_Buffer / Uart_Buffer_Put_frame / Uart_Buffer_Get_frame：环形缓冲区辅助。
- rd_u32_be / wr_u32_be / rd_f32_be / wr_f32_be：大端序读写工具。

## 注意事项
- MAX_PAYLOAD_LEN 与 MAX_CMD_TYPE_HANDLERS 可按需调整。
- 注册的 pram_length 必须小于 MAX_PAYLOAD_LEN。
- Receive_Uart_Frame 需要传入完整帧（长度为 LEN + 8）。

## 许可证
- 以仓库根目录许可证为准。
