from maix import camera, time, app, http, image, display, network, err, uart
import struct

# ========== WiFi ==========
SSID = "iQOO800"
PASS = "Zhaokaiyyds123."
w = network.wifi.Wifi()
e = w.connect(SSID, PASS, wait=True, timeout=60)
err.check_raise(e, "WiFi connect failed")
ip = w.get_ip()

# ========== 屏幕 ==========
disp = display.Display()
ip_shown = False
print(f"Stream: http://{ip}:8000")

# ========== 摄像头 + 图传 ==========
cam = camera.Camera(640, 480)
stream = http.JpegStreamer()
stream.start()
print("http://{}:{}".format(stream.host(), stream.port()))

# ================================================================
#  【通信协议说明】
#  物理层: MaxiCam UART0(TX=A16,RX=A17) → STM32 UART6
#  帧格式: AA 55 <TYPE> <LEN> <PAYLOAD...> <CHK_H> <CHK_L> 0D 0A
#  校验:   sum(TYPE+LEN+PAYLOAD) & 0xFFFF  (大端)
#
#  帧类型:
#    0x10 — 球位置 (int32, 大端, 单位 cm*100)
#           e.g. 0x00000064 = 100 → 1.00cm (球在中心右侧1cm)
#    0xFF — ACK 帧 (协议保留, 本项目不用)
#
#  【对接学弟视觉检测的接口】
#  替换主循环中的 test counter,
#  调用 send_frame(0x10, struct.pack('>i', ball_pos_cm * 100))
#  其中 ball_pos_cm = (球像素X - 摆杆中心像素X) * 像素到cm系数
# ================================================================

# ========== 协议 UART (TX=A16, RX=A17) ==========
uart_dev = uart.UART("/dev/ttyS0", 115200)

def send_frame(ty: int, payload: bytes):
    buf = bytes([0xAA, 0x55, ty, len(payload)]) + payload
    cs = sum(buf[2:]) & 0xFFFF
    buf += bytes([(cs >> 8) & 0xFF, cs & 0xFF, 0x0D, 0x0A])
    print(f"tx: {buf.hex(' ')}")
    n = uart_dev.write(buf)
    if n != len(buf):
        print(f"UART write fail: {n}/{len(buf)}")

# ========== 主循环 ==========
counter = 0
last_send = time.ticks_ms()

while not app.need_exit():
    img = cam.read()
    img_copy = img.copy()
    img_copy.draw_string(10, 10, f"IP: {ip}",
                    color=image.Color.from_rgb(255, 255, 255))
    img_copy.draw_string(10, 40, f"http://{ip}:8000",
                    color=image.Color.from_rgb(0, 255, 0))

    disp.show(img_copy)
    jpg = img.to_jpeg()
    stream.write(jpg)

    now = time.ticks_ms()
    # if now - last_send >= 200:
    #     last_send = now
    #     counter += 1
    #     send_frame(0x10, struct.pack('>I', counter))

    # time.sleep_ms(33)

# ==== 球检测 (待协议调通后启用) ====
# BEAM_CENTER_X = 320
# PIXEL_TO_CM  = 0.05
# MIN_BLOB_AREA = 30
#
# def find_ball(img):
#     blobs = img.find_blobs(
#         [(0, 80, -128, 127, -128, 127)],
#         area_threshold=MIN_BLOB_AREA, pixels_threshold=MIN_BLOB_AREA, merge=True
#     )
#     if not blobs:
#         return None
#     return max(blobs, key=lambda b: b.area())
