"""
protocol.py
UART 简易协议帧 Python 版本
移植自 protocol.c / protocol.h
作者: whyP762 (原 C 版本)
"""

import struct
import logging
from collections import deque
from typing import Optional, Callable, Union, List
from enum import IntFlag, IntEnum

# ---------- 默认参数 ----------
DEFAULT_FRAME_BUFFER_LEN = 16
DEFAULT_TIMEOUT_THRESHOLD_MS = 200
DEFAULT_MAX_RETRY_TIMES = 3

FRAME_OVERHEAD = 8
PAYLOAD_OFFSET = 4
ACK_TYPE = 0xFF

logger = logging.getLogger("UartProtocol")

# ---------- 标志位 ----------
class Flag(IntFlag):
    NONE = 0
    INITIALIZED = 1 << 0
    ENABLE_ACK = 1 << 1
    WAITING_FOR_ACK = 1 << 2

# ---------- 解析状态机 ----------
class State(IntEnum):
    WAITING_HEADER1 = 0
    WAITING_HEADER2 = 1
    WAITING_TYPE = 2
    WAITING_LENGTH = 3
    RECEIVING_PAYLOAD = 4
    WAITING_CHECKSUM1 = 5
    WAITING_CHECKSUM2 = 6
    WAITING_TAIL1 = 7
    WAITING_TAIL2 = 8

# ---------- 通用工具函数 ----------
def calculate_checksum(data: Union[bytes, bytearray, List[int]]) -> int:
    """计算累加和（低 16 位保留）"""
    s = sum(data)
    return s & 0xFFFF

def rd_u32_be(p: bytes) -> int:
    """大端字节 -> uint32"""
    return struct.unpack('>I', p)[0]

def wr_u32_be(v: int) -> bytes:
    """uint32 -> 大端字节"""
    return struct.pack('>I', v)

def rd_f32_be(p: bytes) -> float:
    """大端字节 -> float（通过 uint32 搬运位模式）"""
    return struct.unpack('>f', p)[0]

def wr_f32_be(f: float) -> bytes:
    """float -> 大端字节"""
    return struct.pack('>f', f)

# ---------- 协议主类 ----------
class UartProtocol:
    def __init__(self,
                 header1: int, header2: int,
                 tail1: int, tail2: int,
                 transmit_func: Callable[[bytes], bool],
                 frame_received_handler: Callable[[int, bytes, int], None],
                 queue_instance: Optional[deque] = None,
                 queue_push: Optional[Callable[[deque, int], bool]] = None,
                 queue_pop: Optional[Callable[[deque], Optional[int]]] = None,
                 timeout_handler: Optional[Callable[[int], None]] = None,
                 get_tick: Optional[Callable[[], int]] = None,
                 frame_buffer_len: int = DEFAULT_FRAME_BUFFER_LEN):

        # 参数存储
        self.header1 = header1
        self.header2 = header2
        self.tail1 = tail1
        self.tail2 = tail2
        self.frame_buffer_len = frame_buffer_len
        self.max_payload_size = frame_buffer_len - FRAME_OVERHEAD

        # 必要参数检查
        if header1 == 0 and header2 == 0 and tail1 == 0 and tail2 == 0:
            raise ValueError("帧头帧尾全为零，初始化失败。")
        if not transmit_func or not callable(transmit_func):
            raise ValueError("发送函数无效。")
        if not frame_received_handler or not callable(frame_received_handler):
            raise ValueError("帧接收回调无效。")

        self.transmit_func = transmit_func
        self.frame_received_handler = frame_received_handler

        # 队列相关：默认使用 deque，并适配 push/pop 接口
        if queue_instance is None:
            queue_instance = deque()
        self.queue = queue_instance

        if queue_push and queue_pop:
            self.queue_push = lambda data: queue_push(self.queue, data)
            self.queue_pop = lambda: queue_pop(self.queue)
        else:
            # 默认 deque 操作
            self.queue_push = lambda data: self._default_queue_push(data)
            self.queue_pop = self._default_queue_pop

        # 可选功能
        self.timeout_handler = timeout_handler
        self.get_tick = get_tick

        # 初始化状态
        self.flags = Flag.NONE
        self.state = State.WAITING_HEADER1
        self.frame_buffer = bytearray(frame_buffer_len)
        self.data_len = 0
        self.payload_index = 0
        self.current_frame_type = 0

        # 超时 / ACK 相关
        self.pending_frame_type = 0
        self.last_tick = 0
        self.timeout_threshold = DEFAULT_TIMEOUT_THRESHOLD_MS
        self.try_times = 0
        self.max_try_times = DEFAULT_MAX_RETRY_TIMES
        self.temp_transmit_buffer = bytearray(frame_buffer_len)
        self.temp_buffer_len = 0

        # 判断是否启用 ACK
        ack_enable = True
        if not get_tick or not callable(get_tick):
            logger.info("GetTick 未提供，基于时间的特性将被禁用。")
            ack_enable = False
        if not timeout_handler or not callable(timeout_handler):
            logger.info("超时处理函数未提供，ACK 功能将被禁用。")
            ack_enable = False

        if ack_enable:
            self.flags |= Flag.ENABLE_ACK
        else:
            self.flags &= ~Flag.ENABLE_ACK

        self.flags &= ~Flag.WAITING_FOR_ACK
        self.flags |= Flag.INITIALIZED

        # 状态处理器映射
        self._state_handlers = {
            State.WAITING_HEADER1:   self._handle_header1,
            State.WAITING_HEADER2:   self._handle_header2,
            State.WAITING_TYPE:      self._handle_type,
            State.WAITING_LENGTH:    self._handle_length,
            State.RECEIVING_PAYLOAD: self._handle_payload,
            State.WAITING_CHECKSUM1: self._handle_checksum1,
            State.WAITING_CHECKSUM2: self._handle_checksum2,
            State.WAITING_TAIL1:     self._handle_tail1,
            State.WAITING_TAIL2:     self._handle_tail2,
        }

    # ---------- 内部队列操作 ----------
    def _default_queue_push(self, data: int) -> bool:
        self.queue.append(data)
        return True

    def _default_queue_pop(self) -> Optional[int]:
        try:
            return self.queue.popleft()
        except IndexError:
            return None

    # ---------- 位标志辅助 ----------
    def _set_flag(self, flag: Flag, value: bool):
        if value:
            self.flags |= flag
        else:
            self.flags &= ~flag

    def _is_initialized(self) -> bool:
        return Flag.INITIALIZED in self.flags

    # ---------- 发送原始数据 ----------
    def _transmit_raw(self, data: bytes) -> bool:
        if not self._is_initialized():
            logger.error("协议未初始化，不能发送。")
            return False
        if not self.transmit_func:
            logger.error("发送函数为空。")
            return False
        try:
            return self.transmit_func(data)
        except Exception as e:
            logger.error(f"发送异常: {e}")
            return False

    # ---------- 组帧 ----------
    def _build_frame(self, ty: int, payload: bytes) -> Optional[bytes]:
        """构建完整帧，成功返回 bytes，失败返回 None"""
        if len(payload) > self.max_payload_size:
            logger.error(f"负载长度 {len(payload)} 超过最大值 {self.max_payload_size}")
            return None
        buf = bytearray(FRAME_OVERHEAD + len(payload))
        buf[0] = self.header1
        buf[1] = self.header2
        buf[2] = ty
        buf[3] = len(payload)
        if payload:
            buf[PAYLOAD_OFFSET:PAYLOAD_OFFSET+len(payload)] = payload
        # 校验和计算从索引 2 开始，长度为 payload_len + 2
        checksum = calculate_checksum(buf[2:2 + len(payload) + 2])
        buf[PAYLOAD_OFFSET + len(payload)] = (checksum >> 8) & 0xFF
        buf[PAYLOAD_OFFSET + len(payload) + 1] = checksum & 0xFF
        buf[PAYLOAD_OFFSET + len(payload) + 2] = self.tail1
        buf[PAYLOAD_OFFSET + len(payload) + 3] = self.tail2
        return bytes(buf)

    def _build_ack_frame(self) -> Optional[bytes]:
        """构建 ACK 帧"""
        return self._build_frame(ACK_TYPE, b'')

    # ---------- 发送 ACK ----------
    def _transmit_ack(self) -> bool:
        frame = self._build_ack_frame()
        if frame is None:
            return False
        return self._transmit_raw(frame)

    # ---------- 发送数据帧(内部) ----------
    def _transmit_data(self, ty: int, payload: bytes) -> bool:
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        frame = self._build_frame(ty, payload)
        if frame is None:
            return False

        success = self._transmit_raw(frame)
        if not success:
            logger.error(f"发送帧 type=0x{ty:02X} 失败。")
            if ty != ACK_TYPE:
                self._stop_waiting_ack()
            return False

        # 发送成功后，如果是普通数据帧，进入等待 ACK 状态
        if ty != ACK_TYPE:
            self.pending_frame_type = ty
            self._begin_waiting_ack()
            # 备份发送的数据（完整帧）以便重传
            self.temp_transmit_buffer[:len(frame)] = frame
            self.temp_buffer_len = len(frame)
        return True

    # ---------- 公开发送接口 ----------
    def transmit_frame(self, ty: int, payload: bytes) -> bool:
        """发送普通数据帧（ty 不可为 ACK_TYPE）"""
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        if ty == ACK_TYPE:
            logger.warning(f"帧类型 0x{ACK_TYPE:02X} 保留给 ACK，请使用其他类型。")
            return False
        if len(payload) > self.max_payload_size:
            logger.error(f"负载长度超限: {len(payload)} > {self.max_payload_size}")
            return False
        return self._transmit_data(ty, payload)

    # ---------- ACK 等待与超时管理 ----------
    def _begin_waiting_ack(self):
        if Flag.ENABLE_ACK not in self.flags:
            return
        self._set_flag(Flag.WAITING_FOR_ACK, True)
        self.try_times = 0
        if self.get_tick:
            self.last_tick = self.get_tick()

    def _stop_waiting_ack(self):
        self._set_flag(Flag.WAITING_FOR_ACK, False)
        self.try_times = 0
        self.temp_buffer_len = 0
        self.pending_frame_type = 0
        self.temp_transmit_buffer[:] = b'\x00' * len(self.temp_transmit_buffer)

    def _check_elapsed_timeout(self):
        """由 loop 调用，检查是否超时并重传"""
        if (Flag.ENABLE_ACK not in self.flags or
            Flag.WAITING_FOR_ACK not in self.flags):
            return False
        if not self.get_tick:
            return False

        now = self.get_tick()
        elapsed = now - self.last_tick
        if elapsed > self.timeout_threshold:
            self.last_tick = now
            logger.debug(f"超时 {elapsed}ms，尝试重传，次数 {self.try_times}")
            return self._retry()
        return False

    def _retry(self):
        if self.try_times < self.max_try_times:
            self.try_times += 1
            # 重传上次的完整帧
            data = bytes(self.temp_transmit_buffer[:self.temp_buffer_len])
            return self._transmit_raw(data)
        else:
            pending = self.pending_frame_type
            self._stop_waiting_ack()
            logger.warning(f"重试次数达上限，放弃等待 ACK，帧类型 0x{pending:02X}")
            if self.timeout_handler:
                self.timeout_handler(pending)
            return False

    # ---------- 状态机各状态处理 ----------
    def _reset_state(self):
        self.data_len = 0
        self.payload_index = 0
        self.current_frame_type = 0
        self.state = State.WAITING_HEADER1

    def _handle_header1(self, data: int) -> bool:
        if data != self.header1:
            logger.debug(f"等待 Header1 时收到异常字节 0x{data:02X}，继续等待。")
            return False
        self.frame_buffer[0] = data
        self.state = State.WAITING_HEADER2
        return True

    def _handle_header2(self, data: int) -> bool:
        if data != self.header2:
            logger.debug(f"等待 Header2 时收到异常字节 0x{data:02X}，重置。")
            self._reset_state()
            return False
        self.frame_buffer[1] = data
        self.state = State.WAITING_TYPE
        return True

    def _handle_type(self, data: int) -> bool:
        self.current_frame_type = data
        self.frame_buffer[2] = data
        self.state = State.WAITING_LENGTH
        return True

    def _handle_length(self, data: int) -> bool:
        self.data_len = data
        if self.data_len > self.max_payload_size:
            logger.debug(f"长度字节 {data} 超出最大负载，重置。")
            self._reset_state()
            return False
        self.frame_buffer[3] = data
        self.payload_index = PAYLOAD_OFFSET
        self.state = (State.RECEIVING_PAYLOAD if self.data_len > 0
                      else State.WAITING_CHECKSUM1)
        return True

    def _handle_payload(self, data: int) -> bool:
        self.frame_buffer[self.payload_index] = data
        self.payload_index += 1
        if self.payload_index >= self.data_len + PAYLOAD_OFFSET:
            self.state = State.WAITING_CHECKSUM1
        return True

    def _handle_checksum1(self, data: int) -> bool:
        self.frame_buffer[self.payload_index] = data
        self.payload_index += 1
        self.state = State.WAITING_CHECKSUM2
        return True

    def _handle_checksum2(self, data: int) -> bool:
        self.frame_buffer[self.payload_index] = data
        self.payload_index += 1
        self.state = State.WAITING_TAIL1
        return True

    def _handle_tail1(self, data: int) -> bool:
        if data != self.tail1:
            logger.debug(f"等待 Tail1 时收到异常字节 0x{data:02X}，重置。")
            self._reset_state()
            return False
        self.frame_buffer[self.payload_index] = data
        self.payload_index += 1
        self.state = State.WAITING_TAIL2
        return True

    def _handle_tail2(self, data: int) -> bool:
        if data != self.tail2:
            logger.debug(f"等待 Tail2 时收到异常字节 0x{data:02X}，重置。")
            self._reset_state()
            return False
        self.frame_buffer[self.payload_index] = data
        # 校验
        checksum_index = self.data_len + PAYLOAD_OFFSET
        received_checksum = (self.frame_buffer[checksum_index] << 8) | self.frame_buffer[checksum_index + 1]
        # 计算从类型到长度+负载 (index 2 ~ 2 + data_len + 2 - 1)
        calc_checksum = calculate_checksum(self.frame_buffer[2:2 + self.data_len + 2])
        if received_checksum != calc_checksum:
            logger.debug(f"校验和不匹配: 收到 0x{received_checksum:04X}, 计算 0x{calc_checksum:04X}")
        else:
            self._deal_frame()
        self._reset_state()
        return True

    # ---------- 处理完整帧 ----------
    def _deal_frame(self):
        ty = self.current_frame_type
        if ty == ACK_TYPE:
            if (Flag.ENABLE_ACK in self.flags and
                Flag.WAITING_FOR_ACK in self.flags):
                self._stop_waiting_ack()
                logger.debug("收到 ACK 帧，退出等待状态。")
            else:
                logger.debug("未在等待 ACK 时收到 ACK 帧，忽略。")
            return

        # 收到数据帧，若启用 ACK 则发送 ACK
        if Flag.ENABLE_ACK in self.flags:
            if not self._transmit_ack():
                logger.warning(f"发送 ACK 失败，帧类型 0x{ty:02X}")

        # 调用用户回调
        if self.frame_received_handler:
            payload = bytes(self.frame_buffer[PAYLOAD_OFFSET:PAYLOAD_OFFSET + self.data_len])
            self.frame_received_handler(ty, payload, self.data_len)

    # ---------- 解析入口（由 loop 调用，每次处理一个字节）----------
    def _analysis(self) -> bool:
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        data = self.queue_pop()
        if data is None:
            return True  # 队列空

        if self.state not in self._state_handlers:
            logger.warning(f"未知状态 {self.state}，重置。")
            self._reset_state()
            return False

        return self._state_handlers[self.state](data)

    # ---------- 公开数据注入 ----------
    def process_byte(self, data: int) -> bool:
        """接收单个字节，放入队列"""
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        return self.queue_push(data & 0xFF)

    def process_buffer(self, data: Union[bytes, bytearray, List[int]]) -> bool:
        """接收多个字节"""
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        for b in data:
            if not self.process_byte(b):
                return False
        return True

    # ---------- 主循环 ----------
    def loop(self) -> bool:
        """每调用一次，检查超时重传，并解析队列中的一个字节"""
        if not self._is_initialized():
            logger.error("协议未初始化。")
            return False
        # 先检查超时
        self._check_elapsed_timeout()
        # 再解析一个字节
        return self._analysis()


# ---------- 使用示例 ----------
if __name__ == "__main__":
    # 配置日志级别
    logging.basicConfig(level=logging.DEBUG)

    # 模拟发送函数
    sent_frames = []
    def mock_transmit(frame: bytes) -> bool:
        sent_frames.append(frame)
        print(f"发送帧: {frame.hex()}")
        return True

    # 接收回调
    received_data = []
    def frame_handler(ty: int, payload: bytes, length: int):
        print(f"收到帧: type=0x{ty:02X}, payload={payload.hex()}")
        received_data.append((ty, payload))

    # 模拟时间 (ms)
    tick = [0]
    def get_tick_ms() -> int:
        return tick[0]

    # 超时回调
    def timeout_cb(ty: int):
        print(f"超时未收到 ACK，帧类型 0x{ty:02X}")

    # 初始化协议
    proto = UartProtocol(
        header1=0xAA, header2=0x55,
        tail1=0x55, tail2=0xAA,
        transmit_func=mock_transmit,
        frame_received_handler=frame_handler,
        get_tick=get_tick_ms,
        timeout_handler=timeout_cb,
        frame_buffer_len=32
    )

    # 1) 向协议中注入一个完整帧： type=0x10, payload = b'\x01\x02\x03'
    payload = bytes([0x01, 0x02, 0x03])
    # 手动构建帧（测试接收）
    frame = proto._build_frame(0x10, payload)
    print(f"注入测试帧: {frame.hex()}")
    proto.process_buffer(frame)

    # 驱动解析
    for _ in range(len(frame)):
        proto.loop()

    # 应能看到“收到帧”打印

    # 2) 测试发送和 ACK 机制（无对端，会超时）
    proto.transmit_frame(0x20, b'\xAA\xBB')
    # 模拟时间流逝，触发重传
    for _ in range(10):
        tick[0] += 220  # 超过阈值 200ms
        proto.loop()
    # 应看到重传尝试，最终超时回调
    