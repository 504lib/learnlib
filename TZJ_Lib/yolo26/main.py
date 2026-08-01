from maix import camera, display, image, nn, app, time, sys, uart, network
from ball_position import (
    AdaptiveAlphaBetaFilter,
    axis_point,
    position_from_pixel,
    validate_calibration,
)
from protocol import UartProtocol
import struct

DEBUG_LOG = False
LOW_LATENCY_MODE = True
LENS_CORR_ENABLE = False

AXIS_START_CM = 0
AXIS_END_CM = 25
DETECTION_CONFIDENCE = 0.4

model_path = "model_224/model_315364.mud"
detector = nn.YOLOv5(model=model_path)

AXIS_START_PX = (5, detector.input_height() // 2 - 25)
AXIS_END_PX = (detector.input_width() - 5, detector.input_height() // 2 - 25)

cam = camera.Camera(detector.input_width(), detector.input_height(), detector.input_format())
disp = display.Display()
position_filter = AdaptiveAlphaBetaFilter()

W, H = detector.input_width(), detector.input_height()

# ==== 启动画面 ====
def _boot_screen(status: str, color=image.COLOR_WHITE):
    """在屏幕上显示连接状态"""
    img = image.Image(W, H, image.Format.FMT_RGB888)
    img.draw_rect(0, 0, W, H, image.Color.from_rgb(0, 0, 0), -1)
    img.draw_string(10, H // 2 - 30, "ZDT Ball Control", image.COLOR_GREEN, 2.0, 2)
    img.draw_string(10, H // 2 + 10, status, color, 1.6, 2)
    disp.show(img)

_boot_screen("Booting...")

# ==== WiFi + WebRTC ====
SSID = "kakakaka"
PASS = "197845047cao"
stream_url = "N/A"
stream_ok = False
wifi_ok = False
stream_start_ms = 0

try:
    _boot_screen("WiFi: connecting...")
    w = network.wifi.Wifi()
    w.connect(SSID, PASS, wait=False)
    t0 = time.ticks_ms()
    ip = "0.0.0.0"
    while time.ticks_ms() - t0 < 8000:
        try:
            ip = w.get_ip()
            if ip and ip != "0.0.0.0":
                wifi_ok = True
                break
        except Exception:
            pass
        time.sleep_ms(300)

    if not wifi_ok:
        _boot_screen(f"WiFi FAILED\nSSID: {SSID}", image.COLOR_RED)
        print("WiFi connect timeout")
        time.sleep(3)
    else:
        _boot_screen(f"WiFi OK: {ip}\nStream starting...")
        from maix import webrtc
        cam2 = cam.add_channel(320, 180, image.Format.FMT_YVU420SP)
        ws = webrtc.WebRTC()
        ws.bind_camera(cam2)
        ws.start()
        raw_url = ws.get_url()
        stream_url = raw_url.replace("0.0.0.0", ip)
        stream_ok = True
        stream_start_ms = time.ticks_ms()
        _boot_screen(f"Stream ready\n{stream_url}", image.COLOR_GREEN)
        print(stream_url)
        time.sleep(2)

except Exception as ex:
    _boot_screen(f"Stream ERROR\n{str(ex)[:40]}", image.COLOR_RED)
    print("Stream failed:", ex)
    time.sleep(3)

# ==== UART ====
uart_dev = uart.UART("/dev/ttyS0", 115200)

def _tx(data: bytes) -> bool:
    return uart_dev.write(data) == len(data)

proto = UartProtocol(
    header1=0xAA, header2=0x55, tail1=0x0D, tail2=0x0A,
    transmit_func=_tx,
    frame_received_handler=lambda ty, p, l: None,
)

validate_calibration(
    AXIS_START_PX, AXIS_END_PX, AXIS_START_CM, AXIS_END_CM,
    detector.input_width(), detector.input_height(),
)

axis_c = image.Color.from_rgb(0, 220, 255)
zero_c = image.Color.from_rgb(255, 220, 0)
panel_c = image.Color.from_rgb(0, 0, 0)
red_c  = image.COLOR_RED
green_c = image.COLOR_GREEN
white_c = image.COLOR_WHITE
yellow_c = image.Color.from_rgb(255, 255, 0)

def draw_axis(img):
    img.draw_line(AXIS_START_PX[0], AXIS_START_PX[1],
                  AXIS_END_PX[0], AXIS_END_PX[1], axis_c, 2)
    img.draw_cross(AXIS_START_PX[0], AXIS_START_PX[1], axis_c, 7, 2)
    img.draw_cross(AXIS_END_PX[0], AXIS_END_PX[1], axis_c, 7, 2)
    mid = (AXIS_START_CM + AXIS_END_CM) / 2
    r = (mid - AXIS_START_CM) / (AXIS_END_CM - AXIS_START_CM)
    zx, zy = axis_point(r, AXIS_START_PX, AXIS_END_PX)
    img.draw_cross(int(zx), int(zy), zero_c, 9, 2)

def draw_status_bar(img):
    """底部状态栏: 推流地址 / 超时提示"""
    bar_y = img.height() - 18
    img.draw_rect(0, bar_y, img.width(), 18, panel_c, -1)

    if stream_ok:
        elapsed = time.ticks_ms() - stream_start_ms
        if elapsed < 60000:  # 2分钟内显示连接中
            status = f"{stream_url}"
        else:
            status = f"{stream_url} [>2min]"
        img.draw_string(2, bar_y + 1, status, green_c, 1.2, 1)
    elif wifi_ok:
        img.draw_string(2, bar_y + 1, "STREAM: wait...", yellow_c, 1.2, 1)
    else:
        img.draw_string(2, bar_y + 1, "NO WIFI | NO STREAM", red_c, 1.2, 1)

last_ms = time.ticks_ms()
loop_ms = 1000

while not app.need_exit():
    loop_ms = time.ticks_ms() - last_ms
    last_ms = time.ticks_ms()

    img = cam.read()
    objs = detector.detect(img, conf_th=DETECTION_CONFIDENCE, iou_th=0.45)
    now_ms = time.ticks_ms()
    draw_axis(img)

    ball = max(objs, key=lambda o: o.score) if objs else None
    if ball is not None:
        rx = ball.x + ball.w * 0.5
        ry = ball.y + ball.h * 0.5
        fx, fy = position_filter.update(rx, ry, now_ms)
        pos_cm, _, _ = position_from_pixel(
            (fx, fy), AXIS_START_PX, AXIS_END_PX,
            AXIS_START_CM, AXIS_END_CM)

        img.draw_rect(ball.x, ball.y, ball.w, ball.h, green_c, 2)
        img.draw_rect(0, 0, detector.input_width(), 35, panel_c, -1)
        img.draw_string(8, 5, f"BALL {pos_cm:+.2f} cm", white_c, 1.4, 2)

        px_per_cm = (AXIS_END_PX[0] - AXIS_START_PX[0]) / 25.0
        vel_cm_s = position_filter.vx / px_per_cm if px_per_cm > 0 else 0.0
        proto.transmit_frame(0x10, struct.pack('>ii',
            int(pos_cm * 10), int(vel_cm_s * 10)))
    else:
        position_filter.mark_missing(now_ms)
        img.draw_rect(0, 0, detector.input_width(), 35, panel_c, -1)
        img.draw_string(8, 5, "BALL LOST", red_c, 1.4, 2)

    draw_status_bar(img)

    fps_str = "FPS:" + str(0 if loop_ms == 0 else 1000 // loop_ms)
    img.draw_string(img.width() - image.string_size(fps_str, 1.4, 2).width(),
                    5, fps_str, green_c, 1.4, 2)

    proto.loop()
    disp.show(img)
