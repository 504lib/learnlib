from maix import camera, time, app, http, image, display, network, err

# ========== WiFi ==========
SSID = "iQOO800"
PASS = "Zhaokaiyyds123."

w = network.wifi.Wifi()
e = w.connect(SSID, PASS, wait=True, timeout=60)
err.check_raise(e, "WiFi connect failed")
ip = w.get_ip()

# ========== 屏幕显示 IP ==========
disp = display.Display()
bg = image.Image(disp.width(), disp.height())
bg.draw_string(10, 10, f"IP: {ip}",
               color=image.Color.from_rgb(255, 255, 255))
bg.draw_string(10, 40, f"http://{ip}:8000",
               color=image.Color.from_rgb(0, 255, 0))
disp.show(bg)

# ========== 摄像头 + 图传 ==========
cam = camera.Camera(640, 480)
stream = http.JpegStreamer()
stream.start()
print("http://{}:{}".format(stream.host(), stream.port()))

# ========== 主循环 ==========
while not app.need_exit():
    img = cam.read()
    # disp.show(img)
    jpg = img.to_jpeg()
    stream.write(jpg)
    time.sleep_ms(33)
