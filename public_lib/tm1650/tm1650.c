/**
 * @file    tm1650.c
 * @brief   TM1650 四位数码管驱动芯片库实现
 * @details
 *          本文件仅包含 TM1650 的 I2C-like 通信协议框架和芯片操作逻辑。
 *          不含任何业务逻辑、应用数据或特定平台的寄存器操作。
 *          所有硬件依赖通过 tm1650_hal_t 回调函数注入。
 * @version 1.0.0
 * @date    2026-07-26
 */

#include "tm1650.h"
#include <string.h>

/*===========================================================================
 * 内部常量
 *===========================================================================*/

/** @brief 数码管位地址表（4位，从左到右） */
static const uint8_t k_digit_addresses[4] = {
    TM1650_DIG1_ADDRESS,    ///< 地址 0x68
    TM1650_DIG2_ADDRESS,    ///< 地址 0x6A
    TM1650_DIG3_ADDRESS,    ///< 地址 0x6C
    TM1650_DIG4_ADDRESS,    ///< 地址 0x6E
};

/** @brief 数字 0-9 的7段码查找表 */
static const uint8_t k_seg_digits[10] = {
    TM1650_SEG_0, TM1650_SEG_1, TM1650_SEG_2, TM1650_SEG_3, TM1650_SEG_4,
    TM1650_SEG_5, TM1650_SEG_6, TM1650_SEG_7, TM1650_SEG_8, TM1650_SEG_9,
};

/** @brief 十六进制 A-F 的7段码查找表 */
static const uint8_t k_seg_hex_alpha[6] = {
    TM1650_SEG_A, TM1650_SEG_B, TM1650_SEG_C,
    TM1650_SEG_D, TM1650_SEG_E, TM1650_SEG_F,
};

/** @brief 内部默认系统配置（亮度1 + 8段模式 + 正常模式 + 显示开） */
#define TM1650_DEFAULT_CONFIG  (TM1650_LV1 | TM1650_8_SEGMENT_MODE | \
                                TM1650_NORMAL_MODE | TM1650_DISPLAY_ON)

/** @brief I2C 通信信号延时（微秒），根据 TM1650 数据手册 t_LOW/t_HIGH ≥ 1μs */
#ifndef TM1650_I2C_DELAY_US
#define TM1650_I2C_DELAY_US  5
#endif

/*===========================================================================
 * 内部辅助宏
 *===========================================================================*/

/** @brief 防御性空指针检查 */
#define CHECK_NULL(ptr)  do { if ((ptr) == NULL) { return false; } } while (0)

/*===========================================================================
 * 硬件抽象层内部辅助函数
 *===========================================================================*/

/**
 * @brief 写 SCK 线电平
 */
static inline void hal_sck_write(const tm1650_hal_t *hal, bool level)
{
    if (hal->pin_write_sck) {
        hal->pin_write_sck(hal->user_data_sck, level);
    }
}

/**
 * @brief 写 DIO 线电平
 */
static inline void hal_dio_write(const tm1650_hal_t *hal, bool level)
{
    if (hal->pin_write_dio) {
        hal->pin_write_dio(hal->user_data_dio, level);
    }
}

/**
 * @brief 读 DIO 线电平
 */
static inline bool hal_dio_read(const tm1650_hal_t *hal)
{
    if (hal->pin_read_dio) {
        return hal->pin_read_dio(hal->user_data_dio);
    }
    return false;
}

/**
 * @brief 微秒延时
 */
static inline void hal_delay_us(const tm1650_hal_t *hal, uint32_t us)
{
    if (hal->delay_us) {
        hal->delay_us(us);
    }
}

/*===========================================================================
 * 低层通信协议（I2C-like 两线串行）
 *===========================================================================*/

/**
 * @brief  发送起始信号
 * @note   SCK为高时，DIO从高变低
 */
static void tm1650_i2c_start(const tm1650_hal_t *hal)
{
    hal_dio_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_sck_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_dio_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
}

/**
 * @brief  发送停止信号
 * @note   SCK为高时，DIO从低变高
 */
static void tm1650_i2c_stop(const tm1650_hal_t *hal)
{
    hal_dio_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_sck_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_dio_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
}

/**
 * @brief  读取应答位（ACK）
 * @param  hal  硬件抽象层接口
 * @return true=收到ACK（DIO=0）, false=未收到ACK（通信异常）
 *
 * @note   DIO设为输入模式后，在第9个SCK时钟读取从机应答
 *         TM1650 在正确接收数据后将 DIO 拉低一个时钟周期
 */
static bool tm1650_i2c_read_ack(const tm1650_hal_t *hal)
{
    bool ack;

    hal_sck_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);

    /* 读取 DIO 状态（此时从机应已将 DIO 拉低） */
    ack = hal_dio_read(hal);

    hal_sck_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_sck_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);

    /* 返回 true = DIO被拉低 = ACK正常 */
    return (ack == false);
}

/**
 * @brief  发送应答位（ACK）
 * @note   主机将 DIO 拉低一个时钟周期表示收到数据
 */
static void tm1650_i2c_send_ack(const tm1650_hal_t *hal)
{
    hal_dio_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_sck_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_sck_write(hal, false);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
    hal_dio_write(hal, true);
    hal_delay_us(hal, TM1650_I2C_DELAY_US);
}

/**
 * @brief  写一个字节（MSB first）
 * @param  hal   硬件抽象层接口
 * @param  data  要写入的字节
 * @return true=收到从机ACK, false=通信失败
 */
static bool tm1650_i2c_write_byte(const tm1650_hal_t *hal, uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        /* 先置DIO，再产生时钟脉冲 */
        hal_dio_write(hal, (data & 0x80) ? true : false);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);

        hal_sck_write(hal, false);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);
        hal_sck_write(hal, true);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);
        hal_sck_write(hal, false);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);

        data <<= 1;
    }

    return tm1650_i2c_read_ack(hal);
}

/**
 * @brief  读一个字节（MSB first）
 * @param  hal  硬件抽象层接口
 * @param  send_ack  读完最后一个bit后是否发送ACK
 * @return 读取到的字节
 */
static uint8_t tm1650_i2c_read_byte(const tm1650_hal_t *hal, bool send_ack)
{
    uint8_t i;
    uint8_t data = 0;

    hal_dio_write(hal, true); /* 释放DIO总线 */
    hal_delay_us(hal, TM1650_I2C_DELAY_US);

    for (i = 0; i < 8; i++) {
        data <<= 1;

        hal_sck_write(hal, true);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);

        if (hal_dio_read(hal)) {
            data |= 0x01;
        }

        hal_sck_write(hal, false);
        hal_delay_us(hal, TM1650_I2C_DELAY_US);
    }

    /* 发送应答 */
    if (send_ack) {
        tm1650_i2c_send_ack(hal);
    }

    return data;
}

/**
 * @brief  发送命令字+数据字（两字节帧）
 * @param  hal   硬件抽象层接口
 * @param  cmd   命令字节
 * @param  data  数据字节
 * @return true=成功, false=通信失败
 *
 * @note   完整帧格式：
 *         START → CMD[7:0] → ACK → DATA[7:0] → ACK → STOP
 */
static bool tm1650_send_command(const tm1650_hal_t *hal,
                                uint8_t cmd, uint8_t data)
{
    bool ok;

    tm1650_i2c_start(hal);
    ok = tm1650_i2c_write_byte(hal, cmd);
    if (!ok) {
        tm1650_i2c_stop(hal);
        return false;
    }
    ok = tm1650_i2c_write_byte(hal, data);
    tm1650_i2c_stop(hal);

    return ok;
}

/**
 * @brief  仅发送一个命令字节
 */
static bool tm1650_send_single_byte(const tm1650_hal_t *hal, uint8_t byte_val)
{
    bool ok;

    tm1650_i2c_start(hal);
    ok = tm1650_i2c_write_byte(hal, byte_val);
    if (!ok) {
        tm1650_i2c_stop(hal);
        return false;
    }
    /* 不发送stop，由调用者控制后续时序 */

    return true;
}

/*===========================================================================
 * 查表函数
 *===========================================================================*/

uint8_t tm1650_seg_for_digit(uint8_t num)
{
    if (num > 9) {
        return TM1650_SEG_BLANK;
    }
    return k_seg_digits[num];
}

uint8_t tm1650_seg_for_hex(uint8_t hex_val)
{
    if (hex_val <= 9) {
        return k_seg_digits[hex_val];
    }
    if (hex_val <= 0x0F) {
        return k_seg_hex_alpha[hex_val - 0x0A];
    }
    return TM1650_SEG_BLANK;
}

uint8_t tm1650_seg_for_char(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return k_seg_digits[(uint8_t)(ch - '0')];
    }
    if (ch >= 'A' && ch <= 'F') {
        return k_seg_hex_alpha[(uint8_t)(ch - 'A')];
    }
    if (ch >= 'a' && ch <= 'f') {
        /* 小写字母用大写段码近似 */
        return k_seg_hex_alpha[(uint8_t)(ch - 'a')];
    }
    switch (ch) {
    case '-': return TM1650_SEG_MINUS;
    case ' ': return TM1650_SEG_BLANK;
    case '_': return 0x08; /* 仅显示 d 段作为下划线 */
    case '=': return 0x48; /* 中段+下段, 似 "≡" */
    default:  return TM1650_SEG_BLANK;
    }
}

/*===========================================================================
 * 初始化与配置
 *===========================================================================*/

bool tm1650_init(tm1650_t *self, const tm1650_hal_t *hal)
{
    CHECK_NULL(self);
    CHECK_NULL(hal);

    /* 拷贝硬件抽象层 */
    memcpy(&self->hal, hal, sizeof(tm1650_hal_t));

    self->system_config = TM1650_DEFAULT_CONFIG;
    self->initialized = true;

    /* 发送默认配置到芯片 */
#ifndef TM1650_SKIP_HW_INIT
    return tm1650_send_command(&self->hal,
                               TM1650_CMD_SYSTEM_CONFIG,
                               self->system_config);
#else
    return true;
#endif
}

bool tm1650_config(tm1650_t *self, uint8_t brightness,
                   uint8_t seg_mode, uint8_t work_mode, uint8_t display_on)
{
    CHECK_NULL(self);
    CHECK_NULL(self->hal.delay_us); /* 至少要有delay函数 */

    self->system_config = brightness | seg_mode | work_mode | display_on;

    return tm1650_send_command(&self->hal,
                               TM1650_CMD_SYSTEM_CONFIG,
                               self->system_config);
}

bool tm1650_set_brightness(tm1650_t *self, uint8_t brightness)
{
    CHECK_NULL(self);

    /* 仅替换亮度位（bit4-6），保留其他配置 */
    self->system_config = (self->system_config & 0x0F) | (brightness & 0x70);

    return tm1650_send_command(&self->hal,
                               TM1650_CMD_SYSTEM_CONFIG,
                               self->system_config);
}

bool tm1650_display_on(tm1650_t *self)
{
    CHECK_NULL(self);

    self->system_config |= TM1650_DISPLAY_ON;

    return tm1650_send_command(&self->hal,
                               TM1650_CMD_SYSTEM_CONFIG,
                               self->system_config);
}

bool tm1650_display_off(tm1650_t *self)
{
    CHECK_NULL(self);

    self->system_config &= ~TM1650_DISPLAY_ON;

    return tm1650_send_command(&self->hal,
                               TM1650_CMD_SYSTEM_CONFIG,
                               self->system_config);
}

/*===========================================================================
 * 低层显示操作
 *===========================================================================*/

bool tm1650_display_segment(tm1650_t *self, uint8_t position, uint8_t seg_data)
{
    CHECK_NULL(self);

    if (position < 1 || position > 4) {
        return false;
    }

    return tm1650_send_command(&self->hal,
                               k_digit_addresses[position - 1],
                               seg_data);
}

bool tm1650_display_segments(tm1650_t *self, uint8_t seg0, uint8_t seg1,
                             uint8_t seg2, uint8_t seg3)
{
    bool ok = true;

    CHECK_NULL(self);

    /* 逐一写入，任一失败则标记但继续执行完 */
    if (!tm1650_send_command(&self->hal, k_digit_addresses[0], seg0)) {
        ok = false;
    }
    if (!tm1650_send_command(&self->hal, k_digit_addresses[1], seg1)) {
        ok = false;
    }
    if (!tm1650_send_command(&self->hal, k_digit_addresses[2], seg2)) {
        ok = false;
    }
    if (!tm1650_send_command(&self->hal, k_digit_addresses[3], seg3)) {
        ok = false;
    }

    return ok;
}

bool tm1650_clear(tm1650_t *self)
{
    CHECK_NULL(self);

    return tm1650_display_segments(self,
                                   TM1650_SEG_BLANK,
                                   TM1650_SEG_BLANK,
                                   TM1650_SEG_BLANK,
                                   TM1650_SEG_BLANK);
}

/*===========================================================================
 * 高层显示操作（自动译码）
 *===========================================================================*/

bool tm1650_display_integer(tm1650_t *self, int16_t number, bool leading_zero)
{
    uint8_t segs[4];
    uint8_t i;
    bool is_negative = false;
    uint16_t abs_num;

    CHECK_NULL(self);

    if (number < 0) {
        if (number < -999) {
            /* 负数越界：显示 "----" */
            return tm1650_display_segments(self,
                                           TM1650_SEG_MINUS,
                                           TM1650_SEG_MINUS,
                                           TM1650_SEG_MINUS,
                                           TM1650_SEG_MINUS);
        }
        is_negative = true;
        abs_num = (uint16_t)(-number);
    } else {
        abs_num = (uint16_t)number;
    }

    if (abs_num > 9999) {
        abs_num = 9999; /* 截断 */
    }

    /* 从最低位（第4位）开始填充，向高位进位 */
    for (i = 4; i > 0; i--) {
        if (abs_num > 0 || i == 4) {
            segs[i - 1] = k_seg_digits[abs_num % 10];
            abs_num /= 10;
        } else {
            segs[i - 1] = leading_zero ? TM1650_SEG_0 : TM1650_SEG_BLANK;
        }
    }

    /* 处理负号：在第1位或第2位显示（如果有前导空白） */
    if (is_negative) {
        if (leading_zero || segs[0] != TM1650_SEG_BLANK) {
            /* 找到最左边非空的位置，在其左边放负号 */
            for (i = 3; i > 0; i--) {
                segs[i] = segs[i - 1];
            }
            segs[0] = TM1650_SEG_MINUS;
        } else {
            /* 找到第一个非空位，在其左边放负号 */
            for (i = 0; i < 3; i++) {
                if (segs[i + 1] != TM1650_SEG_BLANK) {
                    segs[i] = TM1650_SEG_MINUS;
                    break;
                }
            }
        }
    }

    return tm1650_display_segments(self, segs[0], segs[1], segs[2], segs[3]);
}

bool tm1650_display_integer_with_dot(tm1650_t *self,
                                     int16_t number, uint8_t dot_pos)
{
    uint8_t segs[4];
    uint8_t i;
    bool is_negative = false;
    uint16_t abs_num;

    CHECK_NULL(self);

    if (number < 0) {
        is_negative = true;
        abs_num = (uint16_t)(-number);
    } else {
        abs_num = (uint16_t)number;
    }

    if (abs_num > 9999) {
        abs_num = 9999;
    }

    for (i = 4; i > 0; i--) {
        if (abs_num > 0 || i == 4) {
            segs[i - 1] = k_seg_digits[abs_num % 10];
            abs_num /= 10;
        } else if (is_negative && i == 2) {
            segs[i - 1] = TM1650_SEG_0; /* 负号至少占位 */
        } else {
            segs[i - 1] = TM1650_SEG_BLANK;
        }
    }

    if (is_negative) {
        for (i = 3; i > 0; i--) {
            segs[i] = segs[i - 1];
        }
        segs[0] = TM1650_SEG_MINUS;
    }

    /* 加小数点 */
    if (dot_pos >= 1 && dot_pos <= 4) {
        segs[dot_pos - 1] |= TM1650_SEG_DP;
    }

    return tm1650_display_segments(self, segs[0], segs[1], segs[2], segs[3]);
}

bool tm1650_display_string(tm1650_t *self, const char *str)
{
    uint8_t segs[4];
    uint8_t i;

    CHECK_NULL(self);
    CHECK_NULL(str);

    for (i = 0; i < 4; i++) {
        if (str[i] == '\0') {
            /* 剩余位填空 */
            for (; i < 4; i++) {
                segs[i] = TM1650_SEG_BLANK;
            }
            break;
        }
        segs[i] = tm1650_seg_for_char(str[i]);

        /* 检测小数点：如 "1." 这种短字符串，在字符后点亮dp */
        if (str[i] == '.' && i > 0) {
            /* 小数点不单独占一位，而是点亮前一位的dp */
            segs[i - 1] |= TM1650_SEG_DP;
            /* 退一格，用下一个字符填充当前位 */
            segs[i] = (str[i + 1] != '\0')
                           ? tm1650_seg_for_char(str[++i])
                           : TM1650_SEG_BLANK;
        }
    }

    return tm1650_display_segments(self, segs[0], segs[1], segs[2], segs[3]);
}

/*===========================================================================
 * 调试测试
 *===========================================================================*/

void tm1650_test_segments(tm1650_t *self, uint16_t delay_ms)
{
    uint8_t pos, seg;
    uint16_t ms;

    if (!self || !self->hal.delay_us) {
        return;
    }

    /* 逐位逐段点亮跑马灯 */
    const uint8_t test_segs[] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, /* a b c d e f g dp */
    };

    for (seg = 0; seg < 8; seg++) {
        for (pos = 0; pos < 4; pos++) {
            tm1650_send_command(&self->hal,
                                k_digit_addresses[pos],
                                test_segs[seg]);
            for (ms = 0; ms < delay_ms; ms++) {
                hal_delay_us(&self->hal, 1000);
            }
        }
    }

    /* 最后全亮 */
    tm1650_display_segments(self, 0xFF, 0xFF, 0xFF, 0xFF);
}

/*===========================================================================
 * 按键扫描
 *===========================================================================*/

uint8_t tm1650_scan_key(tm1650_t *self)
{
    uint8_t i;
    uint8_t key_value = 0;
    bool ok;

    CHECK_NULL(self);

    /* 1. 发送读按键命令（只发命令字节 + ACK，无数据字节） */
    tm1650_i2c_start(&self->hal);
    ok = tm1650_i2c_write_byte(&self->hal, TM1650_CMD_READ_KEYPAD);
    if (!ok) {
        tm1650_i2c_stop(&self->hal);
        return 0;
    }

    /* 2. 读取ACK后，切换DIO为输入，读取8位按键数据 */
    /* TM1650 将在接下来8个时钟输出按键状态 */
    for (i = 0; i < 8; i++) {
        key_value <<= 1;

        self->hal.pin_write_dio(self->hal.user_data_dio, true); /* 释放总线 */

        hal_delay_us(&self->hal, TM1650_I2C_DELAY_US);
        self->hal.pin_write_sck(self->hal.user_data_sck, true);
        hal_delay_us(&self->hal, TM1650_I2C_DELAY_US);

        if (hal_dio_read(&self->hal)) {
            key_value |= 0x01;
        }

        self->hal.pin_write_sck(self->hal.user_data_sck, false);
        hal_delay_us(&self->hal, TM1650_I2C_DELAY_US);
    }

    /* 3. 发送应答并结束帧 */
    tm1650_i2c_send_ack(&self->hal);
    tm1650_i2c_stop(&self->hal);

    /* 4. 恢复显示配置（读按键操作会扰乱显示，需要恢复） */
    tm1650_send_command(&self->hal,
                        TM1650_CMD_SYSTEM_CONFIG,
                        self->system_config);

    /* 返回低4位（对应KEY1-KEY4） */
    return key_value & 0x0F;
}
