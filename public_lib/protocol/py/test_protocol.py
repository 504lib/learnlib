import logging
import time
from collections import deque
from protocol import UartProtocol, Flag  # 导入 Flag 枚举

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(name)s] %(levelname)s: %(message)s',
    datefmt='%H:%M:%S'
)

# ================ 虚拟物理链路 ================
class VirtualLink:
    def __init__(self):
        self.a_to_b = deque()
        self.b_to_a = deque()

    def send_a2b(self, byte: int) -> bool:
        self.a_to_b.append(byte)
        return True

    def send_b2a(self, byte: int) -> bool:
        self.b_to_a.append(byte)
        return True

    def recv_b_from_a(self):
        try: return self.a_to_b.popleft()
        except IndexError: return None

    def recv_a_from_b(self):
        try: return self.b_to_a.popleft()
        except IndexError: return None

# ================ 模拟设备 ================
class MockDevice:
    def __init__(self, name: str, link: VirtualLink, is_a: bool = True):
        self.name = name
        self.link = link
        self.received_frames = []
        self.tick_ms = 0

        if is_a:
            self.transmit_func = lambda data: all(link.send_a2b(b) for b in data)
            self.feed = lambda: self._feed_from(link.recv_a_from_b)
        else:
            self.transmit_func = lambda data: all(link.send_b2a(b) for b in data)
            self.feed = lambda: self._feed_from(link.recv_b_from_a)

        def frame_handler(ty: int, payload: bytes, length: int):
            self.received_frames.append((ty, payload))
            print(f"[{self.name}] 收到数据帧: type=0x{ty:02X}, payload={payload.hex()}")

        def timeout_handler(ty: int):
            print(f"[{self.name}] ⚠️ 超时！未收到帧类型 0x{ty:02X} 的 ACK")

        self.protocol = UartProtocol(
            header1=0xAA, header2=0x55,
            tail1=0x55, tail2=0xAA,
            transmit_func=self.transmit_func,
            frame_received_handler=frame_handler,
            get_tick=self._get_tick,
            timeout_handler=timeout_handler,
            frame_buffer_len=64
        )

    def _get_tick(self) -> int:
        return self.tick_ms

    def _feed_from(self, recv_func):
        data = recv_func()
        if data is not None:
            self.protocol.process_byte(data)

    def advance_time(self, ms: int):
        self.tick_ms += ms

    def send_frame(self, ty: int, payload: bytes):
        print(f"[{self.name}] 发送数据帧: type=0x{ty:02X}, payload={payload.hex()}")
        return self.protocol.transmit_frame(ty, payload)

    def poll(self):
        self.feed()
        self.protocol.loop()

# ================ 测试用例 ================
def run_tests():
    print("=" * 60)
    print("开始测试 UartProtocol Python 库")
    print("=" * 60)

    # ---------- 测试1 ----------
    print("\n>>> 测试1：A 发送数据帧，B 回复 ACK，A 正常收到 ACK")
    link1 = VirtualLink()
    dev_a1 = MockDevice("Device-A", link1, is_a=True)
    dev_b1 = MockDevice("Device-B", link1, is_a=False)

    dev_a1.send_frame(0x10, bytes([0x01, 0x02, 0x03]))
    for _ in range(20):
        dev_a1.poll()
        dev_b1.poll()
        dev_a1.advance_time(5)
        dev_b1.advance_time(5)
        if not (dev_a1.protocol.flags & Flag.WAITING_FOR_ACK):
            break
        time.sleep(0.01)

    assert len(dev_b1.received_frames) == 1
    assert dev_b1.received_frames[0] == (0x10, bytes([0x01, 0x02, 0x03]))
    print("[PASS] 基础收发正常")

    # ---------- 测试2 ----------
    print("\n>>> 测试2：模拟 ACK 丢失，A 应重传，最终触发超时回调")
    link2 = VirtualLink()
    dev_a2 = MockDevice("Device-A", link2, is_a=True)
    dev_b2 = MockDevice("Device-B", link2, is_a=False)

    original_transmit_b = dev_b2.transmit_func
    dev_b2.transmit_func = lambda data: False

    dev_a2.send_frame(0x20, bytes([0xAA, 0xBB]))

    timeout_triggered = False
    for i in range(20):
        dev_a2.advance_time(220)
        dev_b2.advance_time(220)
        for _ in range(5):
            dev_a2.poll()
            dev_b2.poll()
        if not (dev_a2.protocol.flags & Flag.WAITING_FOR_ACK):
            timeout_triggered = True
            break

    dev_b2.transmit_func = original_transmit_b
    assert timeout_triggered, "A 未触发超时停止等待 ACK"
    print("[PASS] 超时重传机制正常")

    # ---------- 测试3 ----------
    print("\n>>> 测试3：大端序列化 / 反序列化工具")
    from protocol import rd_u32_be, wr_u32_be, rd_f32_be, wr_f32_be

    val = 0x12345678
    assert rd_u32_be(wr_u32_be(val)) == val
    print(f"[PASS] uint32 转换: {val:#x}")

    fval = 3.14159
    assert abs(rd_f32_be(wr_f32_be(fval)) - fval) < 1e-5
    print(f"[PASS] float 转换: {fval} -> {rd_f32_be(wr_f32_be(fval))}")

    # ---------- 测试4 ----------
    print("\n>>> 测试4：发送校验和错误的帧，应被丢弃")
    link4 = VirtualLink()
    dev_a4 = MockDevice("Device-A", link4, is_a=True)
    dev_b4 = MockDevice("Device-B", link4, is_a=False)

    correct_frame = dev_a4.protocol._build_frame(0x30, b'\xDE\xAD')
    bad_frame = bytearray(correct_frame)
    bad_frame[-4] = (bad_frame[-4] + 1) & 0xFF
    dev_b4.protocol.process_buffer(bytes(bad_frame))
    for _ in range(len(bad_frame)):
        dev_b4.poll()

    assert len(dev_b4.received_frames) == 0, "错误校验帧应被丢弃"
    print("[PASS] 错误校验帧被正确丢弃")

    # ---------- 测试5 ----------
    print("\n>>> 测试5：零负载数据帧的发送与接收")
    link5 = VirtualLink()
    dev_a5 = MockDevice("Device-A", link5, is_a=True)
    dev_b5 = MockDevice("Device-B", link5, is_a=False)

    dev_a5.send_frame(0x40, b'')
    for _ in range(30):
        dev_a5.poll()
        dev_b5.poll()
        dev_a5.advance_time(5)
        dev_b5.advance_time(5)
    assert len(dev_b5.received_frames) == 1, "零负载帧应只收到一帧"
    assert dev_b5.received_frames[0] == (0x40, b'')
    print("[PASS] 零负载帧收发正常")

    print("\n" + "=" * 60)
    print("所有测试通过 ✅")
    print("=" * 60)

if __name__ == "__main__":
    run_tests()