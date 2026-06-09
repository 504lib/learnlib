/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
#include "esp_lcd_touch_ft5x06.h"
#endif

static const char *TAG = "example";

// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           14
#define EXAMPLE_PIN_NUM_MOSI           13
#define EXAMPLE_PIN_NUM_MISO           12
#define EXAMPLE_PIN_NUM_LCD_DC         2
#define EXAMPLE_PIN_NUM_LCD_RST        4
#define EXAMPLE_PIN_NUM_LCD_CS         15
#define EXAMPLE_PIN_NUM_BK_LIGHT       21

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_DRAW_BUF_LINES    50 // larger chunk = fewer DMA splits
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

// extern void example_lvgl_demo_ui(lv_disp_t *disp);  // 原例程的旋转弧形+按钮 demo，替换为计数器 UI

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

/*
// 屏幕旋转回调 — 已移除，因为每帧都调用会覆盖 app_main 里设置的镜像/交换参数。
// 如果将来需要旋转功能，把下面的 switch 逻辑移回来，并在 flush_cb 里调用。
static void example_lvgl_port_update_callback(lv_display_t *disp)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    switch (rotation) {
    case LV_DISPLAY_ROTATION_0:   esp_lcd_panel_swap_xy(panel_handle, false); esp_lcd_panel_mirror(panel_handle, true, false); break;
    case LV_DISPLAY_ROTATION_90:  esp_lcd_panel_swap_xy(panel_handle, true);  esp_lcd_panel_mirror(panel_handle, true, true);  break;
    case LV_DISPLAY_ROTATION_180: esp_lcd_panel_swap_xy(panel_handle, false); esp_lcd_panel_mirror(panel_handle, false, true); break;
    case LV_DISPLAY_ROTATION_270: esp_lcd_panel_swap_xy(panel_handle, true);  esp_lcd_panel_mirror(panel_handle, false, false); break;
    }
}
*/

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
}

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
static void example_lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_point_data_t point_data[1] = {0};
    uint8_t touchpad_cnt = 0;

    esp_lcd_touch_handle_t touch_pad = lv_indev_get_user_data(indev);
    esp_lcd_touch_read_data(touch_pad);  // read from hardware first!
    esp_err_t err = esp_lcd_touch_get_data(touch_pad, point_data, &touchpad_cnt, 1);

    if (err == ESP_OK && touchpad_cnt > 0) {
        data->point.x = point_data[0].x;
        data->point.y = point_data[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

static int counter = 0;

static void counter_btn_cb(lv_event_t *e)
{
    counter++;
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "%d", counter);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, EXAMPLE_LVGL_TASK_MIN_DELAY_MS);
        // in case of lvgl display not ready yet
        time_till_next_ms = MIN(time_till_next_ms, EXAMPLE_LVGL_TASK_MAX_DELAY_MS);
        usleep(1000 * time_till_next_ms);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    // MSP2833/MSP2834 custom init command data
    static const uint8_t cmd_CF[] = {0x00, 0xC1, 0x30};
    static const uint8_t cmd_ED[] = {0x64, 0x03, 0x12, 0x81};
    static const uint8_t cmd_E8[] = {0x85, 0x00, 0x78};
    static const uint8_t cmd_CB[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    static const uint8_t cmd_F7[] = {0x20};
    static const uint8_t cmd_EA[] = {0x00, 0x00};
    static const uint8_t cmd_C0[] = {0x13};
    static const uint8_t cmd_C1[] = {0x13};
    static const uint8_t cmd_C5[] = {0x22, 0x35};
    static const uint8_t cmd_C7[] = {0xBD};
    static const uint8_t cmd_B6[] = {0x0A, 0xA2};
    static const uint8_t cmd_F6[] = {0x01, 0x30};
    static const uint8_t cmd_B1[] = {0x00, 0x1B};
    static const uint8_t cmd_F2[] = {0x00};
    static const uint8_t cmd_26[] = {0x01};
    static const uint8_t cmd_E0[] = {0x0F, 0x35, 0x31, 0x0B, 0x0E, 0x06, 0x49, 0xA7, 0x33, 0x07, 0x0F, 0x03, 0x0C, 0x0A, 0x00};
    static const uint8_t cmd_E1[] = {0x00, 0x0A, 0x0F, 0x04, 0x11, 0x08, 0x36, 0x58, 0x4D, 0x07, 0x10, 0x0C, 0x32, 0x34, 0x0F};

    static const ili9341_lcd_init_cmd_t msp2833_init_cmds[] = {
        {0xCF, cmd_CF, 3, 0},       // Power Control B
        {0xED, cmd_ED, 4, 0},       // Power On Sequence Control
        {0xE8, cmd_E8, 3, 0},       // Driver Timing Control A
        {0xCB, cmd_CB, 5, 0},       // Power Control A
        {0xF7, cmd_F7, 1, 0},       // Pump Ratio Control
        {0xEA, cmd_EA, 2, 0},       // Driver Timing Control B
        {0xC0, cmd_C0, 1, 0},       // Power Control 1 (VRH=4.65V)
        {0xC1, cmd_C1, 1, 0},       // Power Control 2
        {0xC5, cmd_C5, 2, 0},       // VCOM Control 1
        {0xC7, cmd_C7, 1, 0},       // VCOM Control 2
        {0x21, NULL, 0, 0},         // Display Inversion ON
        {0xB6, cmd_B6, 2, 0},       // Display Function Control
        {0xF6, cmd_F6, 2, 0},       // Interface Control (CRITICAL!)
        {0xB1, cmd_B1, 2, 0},       // Frame Rate Control
        {0xF2, cmd_F2, 1, 0},       // 3Gamma Function Disable
        {0x26, cmd_26, 1, 0},       // Gamma Curve Selected
        {0xE0, cmd_E0, 15, 0},      // Positive Gamma Correction
        {0xE1, cmd_E1, 15, 0},      // Negative Gamma Correction
    };

    ili9341_vendor_config_t vendor_cfg = {
        .init_cmds = msp2833_init_cmds,
        .init_cmds_size = sizeof(msp2833_init_cmds) / sizeof(ili9341_lcd_init_cmd_t),
    };

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_cfg,
    };
    ESP_LOGI(TAG, "Install ILI9341 panel driver (MSP2833 init)");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // Mirror X for correct orientation (no swap)
    // combo 1: mirror(false,false)  combo 2: mirror(true,false)
    // combo 3: mirror(false,true)   combo 4: mirror(true,true)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    // Color test done — display init confirmed working
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // create a lvgl display
    lv_display_t *display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);

    void *buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf1);
    void *buf2 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

    // I2C pins hardcoded for ESP32-S3: SCL=1, SDA=3, RST=5
    ESP_LOGI(TAG, "Initialize I2C bus for touch controller");
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 6,
        .scl_io_num = 5,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_handle));

    ESP_LOGI(TAG, "Install touch panel IO (I2C)");
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = 7,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 1,
        },
    };
    esp_lcd_touch_handle_t tp = NULL;

    ESP_LOGI(TAG, "Initialize touch controller FT5x06");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

    static lv_indev_t *indev;
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, display);
    lv_indev_set_user_data(indev, tp);
    lv_indev_set_read_cb(indev, example_lvgl_touch_cb);

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    // Counter UI: button press increments count
    ESP_LOGI(TAG, "Creating counter UI");
    _lock_acquire(&lvgl_api_lock);
    lv_obj_t *scr = lv_display_get_screen_active(display);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);

    // Count label
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "0");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);

    // "+1" button
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 120, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "+1");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(btn, counter_btn_cb, LV_EVENT_CLICKED, label);

    _lock_release(&lvgl_api_lock);
}
