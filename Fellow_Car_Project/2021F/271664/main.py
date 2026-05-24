# main.py
from maix import camera, display, image, nn, app
from machine import UART
import os, time


# --- 全局配置与常量 ---
# K210 与 STM32 通过 UART 直连（PA2/PA3 ←→ K210 UART引脚）
UART_INDEX = 2         # K210 UART 编号
UART_BAUD  = 115200    # 与 STM32 USART2 波特率一致

# 协议帧常量（与 MCU 协议层一致）
FRAME_H1 = 0xAA
FRAME_H2 = 0x55
FRAME_T1 = 0x55
FRAME_T2 = 0xAA

# 命令码（与 MCU Task_FSM 对齐）
CMD_STRAIGHT  = 0x01   # 直行 → FSM: TASK_GO_TO_CROSS
CMD_LEFT      = 0x02   # 左转 → FSM: TASK_TURN_LEFT
CMD_RIGHT     = 0x03   # 右转 → FSM: TASK_TURN_RIGHT
CMD_KEY2_LONG = 0x10   # MCU 发来的 key4 长按信号

TARGET_NUMBER = None      # 目标病房号，由RECORD状态自动记录
CURRENT_NUMBER = 0

# --- 状态机枚举与变量 ---
class CamState:
    START         = 0     # 初始化
    RECORD        = 100   # 记录第一个识别到的数字作为目标
    WAIT_KEY      = 120   # 等待单片机key2长按信号（手动控制前进时机）
    SEND_STRAIGHT = 150   # 发送直行指令，小车开始前进
    SEARCH        = 200   # 在路上寻找目标数字
    DETECT        = 300   # 识别到数字，判断是否为目标
    SEND          = 400   # 发送左/右转指令
    END           = 1000  # 任务结束

current_state = CamState.START
target_obj = None
last_detector_time = 0
objs = None
CONFIRM_THRESHOLD = 5
target_confirm_count = 0

# --- 硬件与模型初始化 ---
detector = None
cam = None
dis = None
uart = None

def init_hardware():
    global detector, cam, dis, uart
    print("[STATE:START] 初始化摄像头和模型...")
    model_path = "/root/models/maixhub/271664/model_271664.mud"
    if not os.path.exists(model_path):
        print(f"错误: 模型文件 {model_path} 不存在!")
        return False
    detector = nn.YOLOv5(model=model_path)
    cam = camera.Camera(detector.input_width(), detector.input_height(), detector.input_format())
    dis = display.Display()
    uart = UART(UART_INDEX, UART_BAUD)
    print(f"[STATE:START] UART{UART_INDEX} 初始化完成，波特率 {UART_BAUD}")
    print("[STATE:START] 初始化完成。")
    return True

# --- 协议帧工具函数 ---

def build_frame(cmd_type, payload=b''):
    """构建协议帧: AA 55 type len [payload] checksum(2B BE) 55 AA"""
    hdr = bytes([FRAME_H1, FRAME_H2, cmd_type, len(payload)])
    chk = (cmd_type + len(payload) + sum(payload)) & 0xFFFF
    chk_bytes = bytes([(chk >> 8) & 0xFF, chk & 0xFF])
    tail = bytes([FRAME_T1, FRAME_T2])
    return hdr + payload + chk_bytes + tail

def try_read_frame():
    """从 UART 读取一个协议帧，返回 (type, payload) 或 (None, None)"""
    if uart.any() < 8:
        return None, None
    data = uart.read()
    if not data or len(data) < 8:
        return None, None
    for i in range(len(data) - 7):
        if data[i] == FRAME_H1 and data[i+1] == FRAME_H2:
            ftype = data[i+2]
            flen = data[i+3]
            total_len = 4 + flen + 2 + 2  # hdr(4) + payload + checksum(2) + footer(2)
            if i + total_len > len(data):
                continue
            if data[i+4+flen+2] == FRAME_T1 and data[i+4+flen+3] == FRAME_T2:
                payload = data[i+4:i+4+flen] if flen > 0 else b''
                return ftype, payload
    return None, None

# --- 业务功能函数 ---

def send_command_to_mcu(cmd_type, payload=b''):
    """通过协议帧发送指令给单片机"""
    try:
        frame = build_frame(cmd_type, payload)
        uart.write(frame)
        names = {CMD_STRAIGHT: "直行", CMD_LEFT: "左转", CMD_RIGHT: "右转"}
        name = names.get(cmd_type, f"0x{cmd_type:02X}")
        print(f"[SEND] 发送{name}指令 (type=0x{cmd_type:02X})")
        return True
    except Exception as e:
        print(f"[ERROR] 串口发送失败: {e}")
        return False

def check_mcu_signal():
    """检查是否有来自单片机的key2长按信号，有则返回True"""
    ftype, _ = try_read_frame()
    if ftype == CMD_KEY2_LONG:
        print(f"[RX] 收到 key4 长按信号 (0x{CMD_KEY2_LONG:02X})")
        return True
    elif ftype is not None:
        print(f"[RX] 收到未知帧 type=0x{ftype:02X}")
    return False

def draw_half_screen_lines(img):
    h, w = img.height(), img.width()
    img.draw_line(w // 2, 0, w // 2, h, color=image.COLOR_GREEN, thickness=2)

def check_digit_position(obj, img_width):
    obj_center_x = obj.x + obj.w // 2
    if obj_center_x < img_width // 2:
        return CMD_LEFT
    else:
        return CMD_RIGHT


# --- 主程序 ---
def main():
    global current_state, target_obj, last_detector_time, objs
    global TARGET_NUMBER, CURRENT_NUMBER, target_confirm_count

    while not app.need_exit():
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

        need_detect = current_state in (CamState.RECORD, CamState.SEARCH, CamState.DETECT, CamState.SEND)
        if need_detect and (time.time() - last_detector_time > 0.2):
            objs = detector.detect(img_copy, conf_th=0.7, iou_th=0.45)
            last_detector_time = time.time()

        if target_obj:
            img_copy.draw_rect(target_obj.x, target_obj.y, target_obj.w, target_obj.h, color=image.COLOR_RED)
            msg = f'{detector.labels[target_obj.class_id]}: {target_obj.score:.2f}'
            img_copy.draw_string(target_obj.x, target_obj.y, msg, color=image.COLOR_RED)

        state_names = {
            CamState.START: "START", CamState.RECORD: "RECORD",
            CamState.WAIT_KEY: "WAIT_KEY", CamState.SEND_STRAIGHT: "SEND_STRAIGHT",
            CamState.SEARCH: "SEARCH", CamState.DETECT: "DETECT",
            CamState.SEND: "SEND", CamState.END: "END"
        }
        state_str = state_names.get(current_state, f"UNKNOWN({current_state})")
        img_copy.draw_string(5, 5, f"State:{state_str}", color=image.COLOR_BLUE, scale=1)
        img_copy.draw_string(5, 20, f"Target:{TARGET_NUMBER}", color=image.COLOR_RED, scale=1)
        img_copy.draw_string(5, 35, f"CurNum:{CURRENT_NUMBER}", color=image.COLOR_GREEN, scale=1)

        # ============================================================
        #  状态机
        # ============================================================

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
                    print("[STATE:RECORD] 等待key4长按信号以启动直行...")
                    current_state = CamState.WAIT_KEY
                    continue

        elif current_state == CamState.WAIT_KEY:
            img_copy.draw_string(5, 55, "Press KEY4 long to go straight", color=image.COLOR_YELLOW, scale=1)
            if check_mcu_signal():
                print("[STATE:WAIT_KEY] 收到key4长按信号，准备直行")
                current_state = CamState.SEND_STRAIGHT
                continue

        elif current_state == CamState.SEND_STRAIGHT:
            send_command_to_mcu(CMD_STRAIGHT)
            print(f"[STATE:SEND_STRAIGHT] 已发送直行指令，目标:{TARGET_NUMBER}")
            current_state = CamState.SEARCH
            objs = None
            target_obj = None
            continue

        elif current_state == CamState.SEARCH:
            img_copy.draw_string(5, 55, "Going straight, searching...", color=image.COLOR_YELLOW, scale=1)
            if objs:
                print(f"[STATE:SEARCH] 发现 {len(objs)} 个物体")
                for obj in objs:
                    obj_num = int(detector.labels[obj.class_id])
                    print(f"  检测到数字: {obj_num}")
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
                    cmd = check_digit_position(target_obj, w)
                    print(f"[STATE:DETECT] 目标在{'左' if cmd == CMD_LEFT else '右'}侧!")
                    current_state = CamState.SEND
                    target_confirm_count = 0
            else:
                target_confirm_count = 0
                target_obj = None
                CURRENT_NUMBER = 0
                current_state = CamState.SEARCH
                objs = None
                continue

        elif current_state == CamState.SEND:
            cmd = check_digit_position(target_obj, w)
            if send_command_to_mcu(cmd):
                target_confirm_count = 0
                direction = "左" if cmd == CMD_LEFT else "右"
                print(f"[STATE:SEND] 已发送{direction}转指令，等待下次key4触发")
                current_state = CamState.WAIT_KEY
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
