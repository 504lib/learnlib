# main.py — 使用 protocol.py 的 UartProtocol + maix.uart 原生 API
from maix import camera, display, image, nn, app, uart
from protocol import UartProtocol
import struct, os, time

# --- 配置 ---
UART_DEVICE = "/dev/ttyS0"
UART_BAUD  = 115200
FRAME_H1, FRAME_H2 = 0xAA, 0x55
FRAME_T1, FRAME_T2 = 0x55, 0xAA

# 命令码（与 MCU 一致）
CMD_STRAIGHT      = 0x01
CMD_LEFT          = 0x02
CMD_RIGHT         = 0x03
CMD_KEY2_LONG     = 0x10
CMD_RESET         = 0x20
CMD_CROSS_ARRIVED = 0x30

TARGET_NUMBER = None
CURRENT_NUMBER = 0

# --- 状态机 ---
class CamState:
    START         = 0
    RECORD        = 100
    SEND_STRAIGHT = 150
    WAIT_KEY2     = 160
    SEARCH        = 200
    DETECT        = 300
    SEND          = 400
    WAIT_CROSS    = 450
    END           = 1000

current_state = CamState.START
target_obj = None
last_detector_time = 0
objs = None
CONFIRM_THRESHOLD = 5
target_confirm_count = 0
pending_cmd = None
cross_arrived_flag = False
key2_triggered = False

detector = None
cam = None
dis = None
uart_dev = None
proto = None          # UartProtocol 实例

# --- 协议发送回调 ---
def uart_send(raw_data: bytes) -> bool:
    """UartProtocol 调用此函数发送原始字节"""
    global uart_dev
    try:
        if uart_dev:
            uart_dev.write(raw_data)
            return True
    except Exception as e:
        print(f"[UART] send error: {e}")
    return False

# --- 协议接收回调 ---
def on_frame_received(frame_type: int, payload: bytes, length: int):
    """UartProtocol 收到完整帧时回调"""
    print(f"[RX] frame type=0x{frame_type:02X}, len={length}")
    if frame_type == CMD_KEY2_LONG:
        global key2_triggered
        key2_triggered = True
        print(f"[RX] 收到 key2 触发信号")
    elif frame_type == CMD_RESET:
        print(f"[RX] 收到复位信号")
    elif frame_type == CMD_CROSS_ARRIVED:
        global cross_arrived_flag
        cross_arrived_flag = True
        print(f"[RX] 收到到达路口信号")
    else:
        print(f"[RX] 收到未知帧 type=0x{frame_type:02X}")

# --- 发送指令给 MCU ---
def send_command_to_mcu(cmd_type, payload=b''):
    try:
        proto.transmit_frame(cmd_type, payload)
        names = {CMD_STRAIGHT: "直行", CMD_LEFT: "左转", CMD_RIGHT: "右转"}
        name = names.get(cmd_type, f"0x{cmd_type:02X}")
        print(f"[SEND] 发送{name}指令")
        return True
    except Exception as e:
        print(f"[ERROR] 发送失败: {e}")
        return False

# --- 初始化 ---
def init_hardware():
    global detector, cam, dis, uart_dev, proto
    print("[STATE:START] 初始化摄像头和模型...")
    model_path = "/root/models/maixhub/271664/model_271664.mud"
    if not os.path.exists(model_path):
        print(f"错误: 模型文件 {model_path} 不存在!")
        return False
    detector = nn.YOLOv5(model=model_path)
    cam = camera.Camera(detector.input_width(), detector.input_height(), detector.input_format())
    dis = display.Display()

    # 用 maix.uart 原生 API 打开串口
    try:
        uart_dev = uart.UART(UART_DEVICE, UART_BAUD)
        print(f"[STATE:START] 串口 {UART_DEVICE} 初始化成功, 波特率 {UART_BAUD}")
    except Exception as e:
        print(f"[ERROR] 串口初始化失败: {e}")
        return False

    # 用 protocol.py 的 UartProtocol 接管协议层
    proto = UartProtocol(
        header1=FRAME_H1, header2=FRAME_H2,
        tail1=FRAME_T1, tail2=FRAME_T2,
        transmit_func=uart_send,
        frame_received_handler=on_frame_received
    )
    print("[STATE:START] UartProtocol 初始化成功")
    return True

# --- 辅助函数 ---
def draw_half_screen_lines(img):
    h, w = img.height(), img.width()
    img.draw_line(w // 2, 0, w // 2, h, color=image.COLOR_GREEN, thickness=2)

def check_digit_position(obj, img_width):
    obj_center_x = obj.x + obj.w // 2
    return CMD_LEFT if obj_center_x < img_width // 2 else CMD_RIGHT

# --- 主程序 ---
def main():
    global current_state, target_obj, last_detector_time, objs
    global TARGET_NUMBER, CURRENT_NUMBER, target_confirm_count
    global pending_cmd, cross_arrived_flag, key2_triggered
    global uart_dev, proto

    while not app.need_exit():
        # ---- 从 UART 读取数据，喂给协议层 ----
        if uart_dev and proto:
            data = uart_dev.read()
            if data:
                proto.process_buffer(data)
            # 驱动协议状态机
            proto.loop()

        if cam is None:
            if current_state == CamState.START:
                if not init_hardware():
                    break
                current_state = CamState.RECORD
                time.sleep(0.5)
            else:
                time.sleep(0.1)
            continue

        img = cam.read()
        if img is None:
            continue

        w = img.width()
        img_copy = img.copy()
        draw_half_screen_lines(img_copy)

        need_detect = current_state in (CamState.RECORD, CamState.SEARCH, CamState.DETECT, CamState.SEND, CamState.WAIT_CROSS)
        if need_detect and (time.time() - last_detector_time > 0.2):
            objs = detector.detect(img_copy, conf_th=0.7, iou_th=0.45)
            last_detector_time = time.time()

        if target_obj:
            img_copy.draw_rect(target_obj.x, target_obj.y, target_obj.w, target_obj.h, color=image.COLOR_RED)
            msg = f'{detector.labels[target_obj.class_id]}: {target_obj.score:.2f}'
            img_copy.draw_string(target_obj.x, target_obj.y, msg, color=image.COLOR_RED)

        state_names = {
            CamState.START: "START", CamState.RECORD: "RECORD",
            CamState.SEND_STRAIGHT: "SEND_STRAIGHT", CamState.WAIT_KEY2: "WAIT_KEY2",
            CamState.SEARCH: "SEARCH", CamState.DETECT: "DETECT",
            CamState.SEND: "SEND", CamState.WAIT_CROSS: "WAIT_CROSS", CamState.END: "END"
        }
        img_copy.draw_string(5, 5, f"State:{state_names.get(current_state, '?')}", color=image.COLOR_BLUE, scale=1)
        img_copy.draw_string(5, 20, f"Target:{TARGET_NUMBER}", color=image.COLOR_RED, scale=1)
        img_copy.draw_string(5, 35, f"CurNum:{CURRENT_NUMBER}", color=image.COLOR_GREEN, scale=1)

        # ============ 状态机 ============

        if current_state == CamState.START:
            if not init_hardware():
                break
            current_state = CamState.RECORD
            time.sleep(0.5)
            continue

        elif current_state == CamState.RECORD:
            img_copy.draw_string(5, 55, "Recording target...", color=image.COLOR_YELLOW, scale=1)
            if objs:
                for obj in objs:
                    obj_num = int(detector.labels[obj.class_id])
                    TARGET_NUMBER = obj_num
                    CURRENT_NUMBER = obj_num
                    target_obj = obj
                    print(f"[STATE:RECORD] 记录目标数字: {TARGET_NUMBER}")
                    break
                if TARGET_NUMBER is not None:
                    print("[STATE:RECORD] 目标已记录，发送直行指令")
                    current_state = CamState.SEND_STRAIGHT
                    continue

        elif current_state == CamState.SEND_STRAIGHT:
            send_command_to_mcu(CMD_STRAIGHT)
            print(f"[STATE:SEND_STRAIGHT] 已发送直行指令，目标:{TARGET_NUMBER}")
            current_state = CamState.WAIT_KEY2
            objs = None
            target_obj = None
            continue

        elif current_state == CamState.WAIT_KEY2:
            img_copy.draw_string(5, 55, "Waiting for key2...", color=image.COLOR_YELLOW, scale=1)
            if key2_triggered:
                print("[STATE:WAIT_KEY2] key2 已按下，开始循迹搜索")
                key2_triggered = False
                current_state = CamState.SEARCH
                objs = None
                continue

        elif current_state == CamState.SEARCH:
            img_copy.draw_string(5, 55, "Going straight, searching...", color=image.COLOR_YELLOW, scale=1)
            if objs:
                print(f"[STATE:SEARCH] 发现 {len(objs)} 个物体")
                for obj in objs:
                    print(f"  检测到数字: {int(detector.labels[obj.class_id])}")
                current_state = CamState.DETECT
                continue
            else:
                target_obj = None

        elif current_state == CamState.DETECT:
            if objs is None:
                continue
            found_target = False
            for obj in objs:
                obj_num = int(detector.labels[obj.class_id])
                if obj_num == TARGET_NUMBER:
                    target_obj = obj
                    CURRENT_NUMBER = TARGET_NUMBER
                    found_target = True
                    break
                else:
                    CURRENT_NUMBER = obj_num
                    print(f"[STATE:DETECT] 检测数字:{obj_num}, 目标:{TARGET_NUMBER}")
            if found_target:
                target_confirm_count += 1
                print(f"[STATE:DETECT] 目标确认: {target_confirm_count}/{CONFIRM_THRESHOLD}")
                if target_confirm_count >= CONFIRM_THRESHOLD:
                    pending_cmd = check_digit_position(target_obj, w)
                    print(f"[STATE:DETECT] 目标在{'左' if pending_cmd == CMD_LEFT else '右'}侧! 暂存指令等待路口信号")
                    current_state = CamState.WAIT_CROSS
                    target_confirm_count = 0
            else:
                target_confirm_count = 0
                target_obj = None
                CURRENT_NUMBER = 0
                current_state = CamState.SEARCH
                objs = None
                continue

        elif current_state == CamState.WAIT_CROSS:
            img_copy.draw_string(5, 55, "Waiting for cross signal...", color=image.COLOR_YELLOW, scale=1)
            if cross_arrived_flag:
                if send_command_to_mcu(pending_cmd):
                    direction = "左" if pending_cmd == CMD_LEFT else "右"
                    print(f"[STATE:WAIT_CROSS] 收到路口信号，发送{direction}转指令")
                    cross_arrived_flag = False
                    pending_cmd = None
                    target_confirm_count = 0
                    current_state = CamState.SEARCH
                    objs = None
                    target_obj = None
                else:
                    print("[STATE:WAIT_CROSS] 发送失败，重试...")

        elif current_state == CamState.SEND:
            cmd = check_digit_position(target_obj, w)
            if send_command_to_mcu(cmd):
                target_confirm_count = 0
                direction = "左" if cmd == CMD_LEFT else "右"
                print(f"[STATE:SEND] 已发送{direction}转指令，继续搜索")
                current_state = CamState.SEARCH
                objs = None
                target_obj = None
            else:
                print("[STATE:SEND] 发送失败，重试...")
                continue

        elif current_state == CamState.END:
            img_copy.draw_string(5, 55, "Task complete.", color=image.COLOR_GREEN, scale=1)

        dis.show(img_copy)

    print("程序退出。")

if __name__ == "__main__":
    main()
