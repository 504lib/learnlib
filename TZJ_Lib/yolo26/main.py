from maix import camera, display, image, nn, app, time, sys, uart, touchscreen
from ball_position import (
    AdaptiveAlphaBetaFilter,
    axis_point,
    position_from_pixel,
    validate_calibration,
)
from protocol import UartProtocol
import struct

# 输入打印. 关掉后提升帧率
DEBUG_LOG=False

def print_debug(s):
    if DEBUG_LOG:
        print(s)

# 位置传感器更重视“采集到结果”的延迟，而不是最高吞吐帧率。
# 低延迟模式下 dual_buff=False，每次检测结果都对应本次传入的图像。
# 只有在测试最高检测帧率时，才建议把该开关改为 False。
LOW_LATENCY_MODE = True

# RTSP流
USE_RTSP=False

# HTTP JPEG流
USE_JPEG=True

# WEBRTC流
USE_WEBRTC=False

# 使能畸变校准
LENS_CORR_ENABLE=False

# 畸变校准强度
LENS_CORR_STRENGTH=0.6

# 钢珠位置两点标定。固定摄像头后，把钢珠分别放在两个已知刻度上，
# 将检测框中心像素和实际刻度填写到这里。起点为负方向，终点为正方向。
# 以下像素值仅为初始示例，使用前必须实测。
AXIS_START_PX = (40, 112)
AXIS_END_PX = (280, 112)
AXIS_START_CM = 0       # cm
AXIS_END_CM = 25          # cm
DETECTION_CONFIDENCE = 0.50

# 根据设备选择对应模型；不支持的设备直接报错，避免误加载模型。
# MAIXCAM_MODEL_PATH = "models/yolo26_ball_my_maixcam_maixcam_yolo26/yolo26_ball_my_maixcam.mud"
# MAIXCAM_MODEL_PATH = "models/yolo26_ball_my_maixcam_maixcam_yolo26_480_256/yolo26_ball_my_maixcam.mud"
MAIXCAM_MODEL_PATH = "models/yolo26_all_maixcam_yolo26_480_160/yolo26_all.mud"

MAIXCAM2_MODEL_PATH = "models/yolo26_all_maixcam2_yolo26_640_160/yolo26_all.mud"

def model_path_for_device(device_name):
    """Return the checked-in YOLO26 model matching a Maix device name."""
    normalized_name = device_name.strip().lower()
    if normalized_name == "maixcam2":
        return MAIXCAM2_MODEL_PATH
    if normalized_name in ("maixcam", "maixcam-pro", "maixcam_pro"):
        return MAIXCAM_MODEL_PATH
    raise ValueError("unsupported device: {}".format(device_name))


model = model_path_for_device(sys.device_name())

detector = nn.YOLO26(
    model=model,
    dual_buff=not LOW_LATENCY_MODE,
)

AXIS_START_PX = (5, detector.input_height() // 2 - 40)
AXIS_END_PX = (detector.input_width() - 5, detector.input_height() // 2 - 40)

cam = camera.Camera(detector.input_width(), detector.input_height(), detector.input_format())
disp = display.Display()
ts = touchscreen.TouchScreen()
position_filter = AdaptiveAlphaBetaFilter()

# ---- UART & 协议 ----
uart_dev = uart.UART("/dev/ttyS0", 115200)
def _uart_tx(data: bytes) -> bool:
    n = uart_dev.write(data)
    return n == len(data)
def _on_frame(ty: int, payload: bytes, length: int):
    pass  # 暂不处理接收

proto = UartProtocol(
    header1=0xAA, header2=0x55, tail1=0x0D, tail2=0x0A,
    transmit_func=_uart_tx,
    frame_received_handler=_on_frame,
)

if USE_RTSP:
    from maix import rtsp
    cam2 = cam.add_channel(320, 180, image.Format.FMT_YVU420SP)
    server = rtsp.Rtsp()
    server.bind_camera(cam2)
    server.start()
    print(server.get_url())

if USE_JPEG:
    from maix import http, network
    # 连接热点
    SSID = "iQOO800"
    PASS = "Zhaokaiyyds123."
    ip = "N/A"
    try:
        w = network.wifi.Wifi()
        w.connect(SSID, PASS, wait=False)
        t0 = time.ticks_ms()
        while time.ticks_ms() - t0 < 15000:
            try:
                ip = w.get_ip()
                if ip and ip != "0.0.0.0":
                    break
            except Exception:
                pass
            time.sleep_ms(200)
    except Exception:
        pass
    jpeg_server = http.JpegStreamer()
    jpeg_server.start()
    stream_url = f"http://{ip}:{jpeg_server.port()}"
    print(f"Stream: {stream_url}")

if USE_WEBRTC:
    from maix import webrtc
    cam2 = cam.add_channel(320, 180, image.Format.FMT_YVU420SP)
    server = webrtc.WebRTC()
    server.bind_camera(cam2)
    server.start()
    print(server.get_url())

validate_calibration(
    AXIS_START_PX,
    AXIS_END_PX,
    AXIS_START_CM,
    AXIS_END_CM,
    detector.input_width(),
    detector.input_height(),
)

print(detector.input_width(), detector.input_height())

axis_color = image.Color.from_rgb(0, 220, 255)
zero_color = image.Color.from_rgb(255, 220, 0)
panel_color = image.Color.from_rgb(0, 0, 0)


def draw_calibration_axis(img):
    img.draw_line(
        AXIS_START_PX[0], AXIS_START_PX[1],
        AXIS_END_PX[0], AXIS_END_PX[1],
        axis_color, 2,
    )
    img.draw_cross(AXIS_START_PX[0], AXIS_START_PX[1], axis_color, 7, 2)
    img.draw_cross(AXIS_END_PX[0], AXIS_END_PX[1], axis_color, 7, 2)
    zero_ratio = (0.0 - AXIS_START_CM) / (AXIS_END_CM - AXIS_START_CM)
    zero_x, zero_y = axis_point(zero_ratio, AXIS_START_PX, AXIS_END_PX)
    img.draw_cross(int(zero_x), int(zero_y), zero_color, 9, 2)

last_ms = time.ticks_ms()
loop_ms = 1000
while not app.need_exit():
    loop_ms = time.ticks_ms() - last_ms
    print_debug("loop cost " + str(loop_ms) + "ms")
    last_ms = time.ticks_ms()

    img = cam.read()
    t = time.ticks_ms()

    if LENS_CORR_ENABLE:
        img = img.lens_corr(strength=LENS_CORR_STRENGTH)
    print_debug("lens_corr cost " + str(time.ticks_ms() - t) + "ms")

    t = time.ticks_ms()
    objs = detector.detect(img, conf_th=DETECTION_CONFIDENCE, iou_th=0.45)
    print_debug("detect cost " + str(time.ticks_ms() - t) + "ms")

    t = time.ticks_ms()
    now_ms = time.ticks_ms()
    draw_calibration_axis(img)

    # 模型只有 steel_ball 一个类别。如果一帧出现多个框，只选择置信度最高的框，
    # 避免多个误检框同时干扰位置滤波器。
    ball = max(objs, key=lambda obj: obj.score) if objs else None
    if ball is not None:
        raw_x = ball.x + ball.w * 0.5
        raw_y = ball.y + ball.h * 0.5
        filtered_x, filtered_y = position_filter.update(raw_x, raw_y, now_ms)
        position_cm, axis_ratio, _axis_distance = position_from_pixel(
            (filtered_x, filtered_y),
            AXIS_START_PX,
            AXIS_END_PX,
            AXIS_START_CM,
            AXIS_END_CM,
        )
        projected_x, projected_y = axis_point(
            axis_ratio, AXIS_START_PX, AXIS_END_PX
        )

        img.draw_rect(ball.x, ball.y, ball.w, ball.h, color=image.COLOR_GREEN, thickness=2)

        if DEBUG_LOG:
            img.draw_cross(
                int(filtered_x), int(filtered_y), image.COLOR_GREEN, 12, 2
            )
            img.draw_cross(
                int(projected_x), int(projected_y), zero_color, 7, 2
            )
            msg = (
                f"{detector.labels[ball.class_id]}: {ball.score:.2f} "
                f"px=({filtered_x:.1f},{filtered_y:.1f})"
            )
            img.draw_string(ball.x, ball.y, msg, color=image.COLOR_RED)
            img.draw_rect(0, 0, detector.input_width(), 35, panel_color, -1)
        img.draw_string(
            8, 5, f"BALL {position_cm:+.2f} cm",
            color=image.COLOR_WHITE, scale=1.4, thickness=2,
        )

        # 发送球位置+速度: pos(mm) int32, vel(mm/s) int32 (type 0x10, 8字节)
        px_per_cm = (AXIS_END_PX[0] - AXIS_START_PX[0]) / (AXIS_END_CM - AXIS_START_CM)
        vel_cm_s = position_filter.vx / px_per_cm if px_per_cm > 0 else 0.0
        pos_mm = int(position_cm * 10)
        vel_mm_s = int(vel_cm_s * 10)
        proto.transmit_frame(0x10, struct.pack('>ii', pos_mm, vel_mm_s))
    else:
        position_filter.mark_missing(now_ms)

        img.draw_rect(0, 0, detector.input_width(), 35, panel_color, -1)
        img.draw_string(
            8, 5, "BALL LOST",
            color=image.COLOR_RED, scale=1.4, thickness=2,
        )

    if ip != "N/A":
        img.draw_string(2, img.height() - 18, stream_url, color=image.COLOR_GREEN)
    fps_str = "FPS:" + str(0 if loop_ms == 0 else 1000//loop_ms)
    img.draw_string(
        img.width() - image.string_size(fps_str, scale=1.4, thickness=2).width(), 5, fps_str,
        color=image.COLOR_GREEN, scale=1.4, thickness=2,
    )
    print_debug("draw and get position cost " + str(time.ticks_ms() - t) + "ms")

    if USE_JPEG:
        jpeg_server.write(img)
    proto.loop()
    disp.show(img)
