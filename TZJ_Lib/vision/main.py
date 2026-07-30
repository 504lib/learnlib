"""
5ball — 小钢球混合检测 + 图传 + UART 位置上报
================================================
运行平台: MaixCAM (MaixPy v4)
检测策略: 双路融合
  路1: LAB 颜色 find_blobs — 抓钢球银白反光特征
  路2: YOLOv5 模型检测  — 抓球体形状特征
  两路结果经几何过滤 + IoU 去重后合并输出

数据流:
  摄像头(448×448 RGB)
    ├→ find_blobs(LAB色块) ─┐
    ├→ YOLOv5(model_309453) ─┤
    └→ 几何过滤 + IoU去重 ──→ 红框绘制
                              ├→ 橙框水管参考 + 位置cm显示
                              ├→ UART 200ms/次 发送偏移量
                              └→ HTTP JPEG 图传推流
"""

from maix import camera, time, app, http, image, display, network, err, uart, nn
from maix._maix.image import COLOR_BLACK
from protocol import UartProtocol
import struct
import os

# ============================================================
# 一、WiFi 连接
# ============================================================
SSID = "iQOO800"
PASS = "Zhaokaiyyds123."
w = network.wifi.Wifi()
e = w.connect(SSID, PASS, wait=True, timeout=60)
err.check_raise(e, "WiFi connect failed")
ip = w.get_ip()

# ============================================================
# 二、加载 YOLOv5 模型
# ============================================================
# model_309453: 单类检测 "ball", 输入 448×448 RGB
# 训练平台: MaixHub, 7.0M 参数
model_path = "model_309453.mud"
if not os.path.exists(model_path):
    model_path = "/root/models/maixhub/309453/model_309453.mud"
detector = nn.YOLOv5(model=model_path)
input_w = detector.input_width()   # 模型输入宽度 (448)
input_h = detector.input_height()  # 模型输入高度 (448)
print(f"Model input: {input_w}x{input_h}")

# ============================================================
# 三、摄像头 & 显示屏 & 图传初始化
# ============================================================
# 摄像头按模型输入尺寸打开，确保图像与模型输入尺寸一致
cam = camera.Camera(input_w, input_h, detector.input_format())
disp = display.Display()

# HTTP JPEG 推流 — 浏览器访问 http://<ip>:<port> 即可看到实时画面
stream = http.JpegStreamer()
stream.start()

# ---- 启动画面：显示 IP + 推流地址 ----
# 安全获取颜色值：逐级检查避免 image.Color 不存在时崩溃
try:
    COLOR_WHITE = image.Color.from_rgb(255, 255, 255)
    COLOR_GREEN_VAL = image.Color.from_rgb(0, 255, 0)
except (AttributeError, TypeError):
    COLOR_WHITE = 0xFFFF       # RGB565 白
    COLOR_GREEN_VAL = 0x07E0   # RGB565 绿

stream_url = f"http://{ip}:{stream.port()}"
print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>"+f"IP:  {ip}");
try:
    bg = image.Image(disp.width(), disp.height(), image.Format.FMT_RGB888)
    bg.draw_string(10, 10, f"IP: {ip}", color=COLOR_WHITE)
    bg.draw_string(10, 40, stream_url, color=COLOR_GREEN_VAL)
    disp.show(bg)
except Exception as ex:
    print(f"Startup screen error: {ex}")
print(f"Stream: {stream_url}")

# ============================================================
# 四、UART 串口 & 协议 (TX=A16, RX=A17, 波特率 115200)
# ============================================================
uart_dev = uart.UART("/dev/ttyS0", 115200)

def _uart_transmit(data: bytes) -> bool:
    """UART 发送，返回是否成功"""
    n = uart_dev.write(data)
    if n != len(data):
        print(f"UART write fail: {n}/{len(data)}")
        return False
    return True

def _on_frame_received(ty: int, payload: bytes, length: int):
    """接收到完整帧的回调 (预留，当前仅日志)
    注意: 高频接收时会刷屏，正式使用时应改为节流日志或去掉 print。
    """
    print(f"RX frame: type=0x{ty:02X}, len={length}")

def _on_timeout(ty: int):
    """ACK 超时回调 (未启用 ACK 时不会被调用)"""
    print(f"TX timeout, frame type=0x{ty:02X}")

# 创建协议实例: 帧头 AA 55, 帧尾 0D 0A, 校验和从 TYPE 到 payload 末尾
# ACK 暂未启用 — 如需双向通信 + 自动重传，取消下面两行的注释:
#   get_tick=time.ticks_ms,
#   timeout_handler=_on_timeout,
proto = UartProtocol(
    header1=0xAA, header2=0x55,
    tail1=0x0D, tail2=0x0A,
    transmit_func=_uart_transmit,
    frame_received_handler=_on_frame_received,
)

# ============================================================
# 五、检测参数
# ============================================================

# ———— YOLOv5 参数 ————
CONF_TH = 0.30   # 置信度阈值 (降低以抓小目标，提高以减少误检)
IOU_TH  = 0.45   # NMS 交并比阈值

# ———— find_blobs 颜色阈值 ————
# 关键：find_blobs 的 A/B 范围是 -128 ~ 127 (0=中性灰)
#       get_statistics 的 A/B 范围是   0 ~ 255 (128=中性灰)
#       转换公式: find_blobs_value = get_statistics_value - 128
#
# 钢球光学特征 (LAB):
#   L 高   — 金属反光强烈，亮度高
#   A≈0   — 银白色不含红/绿色调
#   B≈0   — 银白色不含黄/蓝色调
BLOB_THRESHOLDS = [
    # (L_min, L_max, A_min, A_max, B_min, B_max)   A/B: -128~127
    (140, 255,  -18,  18,  -18,  18),   # 强反光镜面高亮区
    (100, 180,  -12,  12,  -12,  12),   # 一般亮度金属灰区
]
BLOB_AREA_MIN = 25     # 最小色块面积 (像素)，过滤噪点 (有意偏小，兼顾远球)
BLOB_AREA_MAX = 32000  # 最大色块面积，过滤大片背景 (448×448 标定)
BLOB_MERGE    = True   # 合并相邻同色色块 (避免一个球碎成多个块)
BLOB_MARGIN   = 5      # 合并时的边缘容差 (像素)

# ———— 几何过滤 ————
# 钢球的几何特征：圆形 (w≈h)，尺寸在一定范围内
# 注意: 尺寸阈值基于 448×448 分辨率标定，更换模型分辨率后需重新调整
#       MIN_SIZE 保持较小值以兼顾远处小球，MAX_SIZE 已按 2x 缩放
MIN_SIZE        = 6     # 最小宽/高 (像素) — 未缩放，远球可能在 10px 以下
MAX_SIZE        = 360   # 最大宽/高 (像素) — 180×2，近球在 448 下可达 ~300px
ASPECT_MIN      = 0.65  # 最小宽高比 (w/h)
ASPECT_MAX      = 1.50  # 最大宽高比 (球理想值为 1.0)
CIRCULARITY_MIN = 0.55  # 最小圆形度 (面积/外接矩形面积, 正圆≈0.785)

# ———— 双路合并 ————
IOU_MERGE_TH = 0.45  # IoU > 此值视为同一目标，去重保留高分

# ———— UART 坐标换算 ————
PIXEL_TO_CM = 0.025  # 像素→厘米换算系数 (448×448, 需根据实际距离标定)

# ============================================================
# 六、颜色常量 (MaixPy 兼容 + RGB565 回退)
# ============================================================
# RGB565 格式: R=5bit, G=6bit, B=5bit
COLOR_RED    = getattr(image, 'COLOR_RED',    0xF800)  # R=31,G=0,B=0  → 球框+分数
COLOR_GREEN  = getattr(image, 'COLOR_GREEN',  0x07E0)  # R=0,G=63,B=0  → 预留
COLOR_BLUE   = getattr(image, 'COLOR_BLUE',   0x001F)  # R=0,G=0,B=31  → 预留
try:
    COLOR_ORANGE = image.Color.from_rgb(255, 128, 0)
except (AttributeError, TypeError):
    COLOR_ORANGE = 0xFC00        # RGB565 橙 (R=31,G=32,B=0)

# ============================================================
# 七、水管参考框
# ============================================================
# 橙色矩形框用于标记 25cm 半圆水管在画面中的区域。
# 调整此四元组使橙框精确套住水管：
#   x — 框左上角横坐标
#   y — 框左上角纵坐标
#   w — 框宽 (对应水管长度方向)
#   h — 框高 (对应水管直径方向)
# 程序将根据 (球心_x - 框左_x) / 框宽 × PIPE_CM 计算球在水管中的位置
PIPE_ROI = (40, 180, 400, 100)   # (x, y, w, h) ⚠️ 448分辨率需重新测量
PIPE_CM  = 25.0               # 水管实际长度 (cm)

# ============================================================
# 八、数据结构 & 工具函数
# ============================================================

class DetObj:
    """
    统一的检测结果对象。
    兼容 find_blobs (source="blob") 和 YOLOv5 (source="yolo") 两路输出。
    """
    def __init__(self, x, y, w, h, score, class_id=0, source="blob"):
        self.x = x           # 边界框左上角 x
        self.y = y           # 边界框左上角 y
        self.w = w           # 边界框宽度
        self.h = h           # 边界框高度
        self.score = score   # 置信度分数 0~1
        self.class_id = class_id  # 类别 ID (当前仅 class 0 = "ball")
        self.source = source      # 来源: "blob" 或 "yolo"


def calc_iou(a: DetObj, b: DetObj) -> float:
    """
    计算两个边界框的 IoU (Intersection over Union)。
    IoU = 交集面积 / 并集面积，范围 0~1。
    用于合并去重：IoU > IOU_MERGE_TH 视为同一目标。
    """
    # 两个框的四条边
    ax1, ay1, ax2, ay2 = a.x, a.y, a.x + a.w, a.y + a.h
    bx1, by1, bx2, by2 = b.x, b.y, b.x + b.w, b.y + b.h
    # 交集区域
    ix1 = max(ax1, bx1)
    iy1 = max(ay1, by1)
    ix2 = min(ax2, bx2)
    iy2 = min(ay2, by2)
    iw = max(0, ix2 - ix1)
    ih = max(0, iy2 - iy1)
    inter = iw * ih
    # 并集面积 = A面积 + B面积 - 交集面积
    area_a = a.w * a.h
    area_b = b.w * b.h
    union = area_a + area_b - inter
    return inter / union if union > 0 else 0


def geometry_check(w: int, h: int, area=None) -> bool:
    """
    几何三重过滤:
      1. 尺寸范围 — 过滤极小噪点和极大背景块
      2. 宽高比   — 球是圆的，w≈h
      3. 圆形度   — 面积填充率 (仅当提供 area 时检查)

    参数:
      w, h: 边界框宽高 (像素)
      area: blob 实际像素面积 (可选，find_blobs 可提供)
    返回:
      True 如果通过几何验证
    """
    if w < MIN_SIZE or h < MIN_SIZE:
        return False
    if w > MAX_SIZE or h > MAX_SIZE:
        return False
    ratio = w / h if h > 0 else 999
    if ratio < ASPECT_MIN or ratio > ASPECT_MAX:
        return False
    # 圆形度 = blob实际面积 / 外接矩形面积
    # 正圆 ≈ 0.785 (π/4)，方形 = 1.0，不规则 < 0.5
    if area is not None:
        circ = area / (w * h)
        if circ < CIRCULARITY_MIN:
            return False
    return True


def color_verify_roi(img, obj: DetObj) -> bool:
    """
    LAB 颜色统计验证 — 检查检测框内区域是否具备钢球光学特征。

    钢球三特征 (基于 get_statistics 的 0~255 范围 LAB 值):
      亮 (bright):   L均值 ≥ 90  — 金属反光不会太暗
      灰 (neutral):  A/B标准差 ≤ 30 — 银白色不含彩色
      闪 (shiny):    L标准差 ≥ 25 — 镜面反射产生高光纹理

    ROI 内缩 20% 排除框边缘的杂乱背景像素。
    """
    margin_x = int(obj.w * 0.2)  # 左右各缩 20%
    margin_y = int(obj.h * 0.2)  # 上下各缩 20%
    rx = max(0, obj.x + margin_x)
    ry = max(0, obj.y + margin_y)
    rw = max(4, obj.w - 2 * margin_x)  # 最小 4px 防止为 0
    rh = max(4, obj.h - 2 * margin_y)

    try:
        # get_statistics 返回 24 个整数的列表:
        #   L[0:8]   A[8:16]  B[16:24]
        #   每通道: [mean, median, mode, std, min, max, Q1, Q3]
        stats = img.get_statistics(roi=(rx, ry, rw, rh))
    except Exception:
        return True  # API 异常则放行，避免漏检

    # stats 布局: [L*8][A*8][B*8]，每通道 [mean, median, mode, std, min, max, Q1, Q3]
    l_mean = stats[0]   # L 均值
    l_std  = stats[3]   # L 标准差
    a_std  = stats[11]  # A 标准差 (偏移 8 + 3)
    b_std  = stats[19]  # B 标准差 (偏移 16 + 3)

    bright  = l_mean >= 90           # 够亮 (反光)
    neutral = a_std <= 30 and b_std <= 30  # 够灰 (无彩色)
    shiny   = l_std >= 25            # 够闪 (有高光)

    return bright and neutral and shiny


# ============================================================
# 九、Blob 兼容访问
# ============================================================
# MaixPy 不同版本的 find_blobs 返回的 blob 对象访问方式可能不同:
#   v4:  blob[0]=x, blob[1]=y, blob[2]=w, blob[3]=h (索引, 文档确认)
#   其他: blob.x, blob.y 等属性或 .x() .y() 等方法
# 以下两个函数自动兼容三种模式。

def blob_xywh(b) -> tuple:
    """从 blob 提取 (x, y, w, h)，兼容索引/属性/方法调用"""
    try:
        return b[0], b[1], b[2], b[3]       # 索引 (文档确认)
    except (TypeError, IndexError):
        pass
    try:
        return b.x, b.y, b.w, b.h           # 属性 (pybind11)
    except AttributeError:
        pass
    try:
        return b.x(), b.y(), b.w(), b.h()   # 方法 (旧版)
    except (TypeError, AttributeError):
        pass
    raise TypeError(f"Cannot extract x,y,w,h from blob: {type(b)}")


def blob_area(b) -> int:
    """提取 blob 像素面积，直接 try/except 避免嵌入式 lambda 性能损耗"""
    try:
        return int(b.pixels)  # pybind11 属性或方法 (int() 兼容两者)
    except (AttributeError, TypeError):
        pass
    try:
        return int(b.area)    # 可能的别名
    except (AttributeError, TypeError):
        pass
    # 回退: 用外接矩形面积 (圆形度检查退化为恒真，但不影响主流程)
    x, y, w, h = blob_xywh(b)
    return w * h


# ============================================================
# 十、检测管线
# ============================================================

# ———— 路1: 颜色 Blob 检测 ————
def detect_blobs(img) -> list:
    """
    LAB 颜色空间找色块。
    核心思路: 钢球是银白反光物体 → L高、A≈0、B≈0。
    对每组合格的 LAB 阈值调用 find_blobs，
    返回通过几何过滤的 DetObj 列表 (source="blob")。

    x_stride=4 在 448×448 下等效于旧版 224×224 的 stride=2，
    保持扫描密度不变的同时控制性能。
    """
    candidates = []
    for th in BLOB_THRESHOLDS:
        try:
            blobs = img.find_blobs(
                [th],                          # 当前 LAB 阈值
                roi=None,                      # 全图搜索
                x_stride=4, y_stride=4,        # 步长 (越大越快，越粗糙; 448分辨率用4保持速度)
                area_threshold=BLOB_AREA_MIN,  # 面积阈值
                pixels_threshold=BLOB_AREA_MIN,# 像素数阈值
                merge=BLOB_MERGE,              # 合并相邻块
                margin=BLOB_MARGIN,            # 合并容差
            )
        except Exception:
            continue  # 某组阈值不适用则跳过
        if not blobs:
            continue

        for b in blobs:
            x, y, w, h = blob_xywh(b)
            area = blob_area(b)

            if area > BLOB_AREA_MAX:
                continue
            if not geometry_check(w, h, area):
                continue

            # 圆形度 → 置信度映射 (正圆 0.785 → 分数 0.9)
            circularity = area / (w * h) if w * h > 0 else 0
            score = min(1.0, circularity / 0.785 * 0.9)

            candidates.append(DetObj(x, y, w, h, score=score, source="blob"))

    return candidates


# ———— 路2: YOLOv5 + 后处理 ————
def filter_yolo_objs(img, objs) -> list:
    """
    YOLOv5 检测结果后处理:
      高置信度 (≥0.65) → 直接信任，跳过颜色验证
      低置信度 (<0.65) → 必须通过颜色验证才保留
      所有结果 → 必须通过几何过滤 (尺寸+宽高比)
    注意: YOLO 不提供像素面积，因此跳过圆形度检查。
          这是有意为之 — YOLO 的形状特征已隐含在检测中，
          而 blob 路径的圆形度检查用于弥补颜色检测的形状盲区。
    """
    valid = []
    for obj in objs:
        # 第一步: 几何过滤 (尺寸 + 宽高比)
        if not geometry_check(obj.w, obj.h):
            continue

        if obj.score >= 0.65:
            # 高置信度: 直接放行
            valid.append(DetObj(
                obj.x, obj.y, obj.w, obj.h,
                score=obj.score, class_id=obj.class_id, source="yolo"
            ))
            continue

        # 低置信度: 颜色验证
        if color_verify_roi(img, obj):
            valid.append(DetObj(
                obj.x, obj.y, obj.w, obj.h,
                score=obj.score, class_id=obj.class_id, source="yolo"
            ))

    return valid


# ———— IoU 去重合并 ————
def merge_deduplicate(blob_candidates: list, yolo_candidates: list) -> list:
    """
    合并两路候选，IoU 去重。
    策略: 按置信度降序排列 → 依次保留 → 跳过与已保留框 IoU > 阈值的框。
    效果: blob 和 YOLO 同时命中同一球时，只保留分数更高的那个。
    """
    all_objs = blob_candidates + yolo_candidates
    if len(all_objs) <= 1:
        return all_objs

    # 按分数降序排列，高分优先保留
    all_objs.sort(key=lambda o: o.score, reverse=True)

    kept = []
    for obj in all_objs:
        duplicate = False
        for k in kept:
            if calc_iou(obj, k) > IOU_MERGE_TH:
                duplicate = True
                break
        if not duplicate:
            kept.append(obj)

    return kept


# ============================================================
# 十一、主循环
# ============================================================
last_send = time.ticks_ms()   # 上次 UART 发送时间
SEND_INTERVAL = 200           # UART 发送间隔 (ms)

print("[5ball] Hybrid detector started: find_blobs + YOLOv5")

while not app.need_exit():
    frame_start = time.ticks_ms()  # 帧耗时起点

    # ———— 读取摄像头帧 ————
    img = cam.read()
    if img is None:          # 摄像头异常保护
        time.sleep_ms(10)
        continue

    # ———— 步骤1: 双路混合检测 ————
    blob_candidates = detect_blobs(img)                          # 路1: 颜色色块
    raw_objs = detector.detect(img, conf_th=CONF_TH, iou_th=IOU_TH)  # 路2: YOLOv5 推理
    yolo_candidates = filter_yolo_objs(img, raw_objs)            # YOLO 后处理
    final_objs = merge_deduplicate(blob_candidates, yolo_candidates)  # 合并去重

    # ———— 步骤2: 绘制水管参考框 ————
    px, py, pw, ph = PIPE_ROI
    img.draw_rect(px, py, pw, ph, color=COLOR_BLACK)
    # 比例换算: 画面中水管宽度 pw 像素 ↔ 实际水管长度 PIPE_CM
    px_to_cm = PIPE_CM / pw if pw > 0 else 0

    # ———— 步骤3: 绘制钢球检测框 ————
    best_obj = None  # 最高分球 (用于 UART 上报 & 水管位置计算)
    for obj in final_objs:
        # 红色边界框
        img.draw_rect(obj.x, obj.y, obj.w, obj.h, color=COLOR_RED)
        # 置信度分数
        msg = f'{obj.score:.2f}'
        img.draw_string(obj.x, obj.y, msg, color=COLOR_RED)
        # 选出最高分球
        if best_obj is None or obj.score > best_obj.score:
            best_obj = obj

    # ———— 步骤4: 水管位置计算 ————
    if best_obj is not None:
        # 球心横坐标
        ball_cx = best_obj.x + best_obj.w / 2
        # 球心距水管左端的实际距离 (cm)
        pos_in_pipe_cm = (ball_cx - px) * px_to_cm
        pipe_msg = f'pipe: {pos_in_pipe_cm:.1f}cm'
        px_msg = f'px:{ball_cx - px}'
        # 显示在水管框正下方
        img.draw_string(px, py + ph + 4, pipe_msg, color=COLOR_ORANGE,scale=2)
        img.draw_string(px, py + ph + 20, px_msg, color=COLOR_ORANGE,scale=2)


    # ———— 步骤5: UART 发送球偏移量 ————
    now = time.ticks_ms()
    if time.ticks_diff(now, last_send) >= SEND_INTERVAL:
        last_send = now
        if best_obj is not None:
            # 计算球心相对画面中心的偏移 (cm)
            center_x = best_obj.x + best_obj.w / 2
            offset_pixel = center_x - img.width() / 2
            offset_cm = offset_pixel * PIXEL_TO_CM
            # 放大 100 倍取整，避免浮点精度问题
            offset_int = int(round(offset_cm * 100))
            payload = struct.pack('>i', offset_int)  # int32 大端
            proto.transmit_frame(0x10, payload)
            print(f"send offset: {offset_cm:.2f} cm ({offset_int})")
        else:
            # 无球时发送 0
            payload = struct.pack('>i', 0)
            proto.transmit_frame(0x10, payload)

    # ———— 步骤5.5: UART 接收处理 ————
    try:
        n = uart_dev.any()
        if n > 0:
            data = uart_dev.read(min(n, 64))
            if data:
                proto.process_buffer(data)
    except Exception:
        pass  # UART 读取失败则跳过，不影响主流程
    proto.loop()  # 驱动协议状态机 (ACK 检查 + 帧解析)

    # ———— 步骤6: 图传推流 & 屏幕显示 ————
    jpg = img.to_jpeg()          # RGB → JPEG 编码
    stream.write(jpg)            # 推送到 HTTP 客户端
    # disp.show(img)               # 本地屏幕显示

    # 帧率控制: 扣除处理耗时后补充 sleep，尽量稳定帧率
    elapsed = time.ticks_ms() - frame_start
    sleep_ms = max(1, 33 - elapsed)
    time.sleep_ms(sleep_ms)
