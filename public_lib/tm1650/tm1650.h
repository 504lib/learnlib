/**
 * @file    tm1650.h
 * @brief   TM1650 四位数码管驱动芯片的跨平台C库
 * @details
 *          TM1650 是一款 I2C-like 两线串行接口的数码管驱动IC，
 *          支持 4位8段/7段数码管显示、8级亮度调节、按键扫描。
 *
 *          本库采用 struct + 函数指针 的硬件抽象设计，
 *          用户只需提供 GPIO 读写和微秒延时的回调函数即可在任意平台使用。
 *
 * @author  Generated based on TM1650 datasheet & reference code
 * @version 1.0.0
 * @date    2026-07-26
 *
 * @copyright Public Domain
 *
 * ================================ 快速开始 ================================
 *
 * 1. 包含头文件:
 *    #include "tm1650.h"
 *
 * 2. 实现三个硬件接口函数:
 *    - void     my_pin_write(void *instance, bool level)
 *    - bool     my_pin_read(void *instance)
 *    - void     my_delay_us(uint32_t us)
 *
 * 3. 填充 tm1650_hal_t 结构体并调用 tm1650_init():
 *    tm1650_t disp;
 *    tm1650_hal_t hal = {
 *        .pin_write_sck = my_pin_write,
 *        .pin_write_dio = my_pin_write,
 *        .pin_read_dio  = my_pin_read,
 *        .delay_us      = my_delay_us,
 *        .user_data_sck = &sck_pin_instance,
 *        .user_data_dio = &dio_pin_instance,
 *    };
 *    tm1650_init(&disp, &hal);
 *
 * 4. 使用高层API显示数字:
 *    tm1650_display_number(&disp, 1234);    // 显示 "1234"
 *    tm1650_set_brightness(&disp, LV4);      // 调到4级亮度
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * 硬件接口回调定义（用户必须实现）
 *===========================================================================*/

/**
 * @brief GPIO 写引脚回调
 * @param user_data  用户自定义的引脚实例指针
 * @param level      true=高电平, false=低电平
 */
typedef void (*tm1650_pin_write_t)(void *user_data, bool level);

/**
 * @brief GPIO 读引脚回调
 * @param user_data  用户自定义的引脚实例指针
 * @return true=高电平, false=低电平
 */
typedef bool (*tm1650_pin_read_t)(void *user_data);

/**
 * @brief 微秒延时回调
 * @param us  延时的微秒数
 */
typedef void (*tm1650_delay_us_t)(uint32_t us);

/*===========================================================================
 * 芯片命令与地址宏定义（勿修改）
 *===========================================================================*/

/** @name 亮度等级
 *  组成系统配置命令的低4位
 *  @{ */
#define TM1650_LV1   0x00   ///< 亮度等级1（最低）
#define TM1650_LV2   0x10   ///< 亮度等级2
#define TM1650_LV3   0x20   ///< 亮度等级3
#define TM1650_LV4   0x30   ///< 亮度等级4
#define TM1650_LV5   0x40   ///< 亮度等级5
#define TM1650_LV6   0x50   ///< 亮度等级6
#define TM1650_LV7   0x60   ///< 亮度等级7
#define TM1650_LV8   0x70   ///< 亮度等级8（最高）
/** @} */

/** @name 段显示模式 */
/** @{ */
#define TM1650_8_SEGMENT_MODE  0x00   ///< 8段显示模式（含小数点）
#define TM1650_7_SEGMENT_MODE  0x08   ///< 7段显示模式（无小数点）
/** @} */

/** @name 工作模式 */
/** @{ */
#define TM1650_NORMAL_MODE     0x00   ///< 正常工作模式
#define TM1650_STANDBY_MODE    0x04   ///< 待机模式（低功耗）
/** @} */

/** @name 显示开关 */
/** @{ */
#define TM1650_DISPLAY_ON      0x01   ///< 打开显示
#define TM1650_DISPLAY_OFF     0x00   ///< 关闭显示
/** @} */

/** @name 命令字节 */
/** @{ */
#define TM1650_CMD_SYSTEM_CONFIG  0x48   ///< 系统配置命令
#define TM1650_CMD_READ_KEYPAD    0x4F   ///< 读按键命令
/** @} */

/** @name 数码管位地址（固定地址模式） */
/** @{ */
#define TM1650_DIG1_ADDRESS   0x68   ///< 第1位数码管（最左）
#define TM1650_DIG2_ADDRESS   0x6A   ///< 第2位数码管
#define TM1650_DIG3_ADDRESS   0x6C   ///< 第3位数码管
#define TM1650_DIG4_ADDRESS   0x6E   ///< 第4位数码管（最右）
/** @} */

/*===========================================================================
 * 段码映射表
 *===========================================================================*/

/**
 * @brief 7段数码管段码表
 * @details
 *  段位映射（8段模式, MSB→LSB）:
 *    [dp] [g] [f] [e] [d] [c] [b] [a]
 *   0x80 0x40 0x20 0x10 0x08 0x04 0x02 0x01
 *
 *  位定义示意（共阴极）:
 *        a
 *      ━━━
 *    f ┃  ┃ b
 *      ━━━  g
 *    e ┃  ┃ c
 *      ━━━  · dp
 *        d
 */

/** @name 数字 0-9 段码 */
/** @{ */
#define TM1650_SEG_0   0x3F   ///< '0'
#define TM1650_SEG_1   0x06   ///< '1'
#define TM1650_SEG_2   0x5B   ///< '2'
#define TM1650_SEG_3   0x4F   ///< '3'
#define TM1650_SEG_4   0x66   ///< '4'
#define TM1650_SEG_5   0x6D   ///< '5'
#define TM1650_SEG_6   0x7D   ///< '6'
#define TM1650_SEG_7   0x07   ///< '7'
#define TM1650_SEG_8   0x7F   ///< '8'
#define TM1650_SEG_9   0x6F   ///< '9'
/** @} */

/** @name 字母 A-F 段码 */
/** @{ */
#define TM1650_SEG_A   0x77   ///< 'A'
#define TM1650_SEG_B   0x7C   ///< 'b'
#define TM1650_SEG_C   0x39   ///< 'C'
#define TM1650_SEG_D   0x5E   ///< 'd'
#define TM1650_SEG_E   0x79   ///< 'E'
#define TM1650_SEG_F   0x71   ///< 'F'
/** @} */

/** @name 常用符号段码 */
/** @{ */
#define TM1650_SEG_MINUS  0x40   ///< '-' 负号（仅中段g）
#define TM1650_SEG_BLANK  0x00   ///< 全灭
#define TM1650_SEG_ALL    0xFF   ///< 全亮（测试用）
/** @} */

/** @name 小数点掩码 */
#define TM1650_SEG_DP     0x80   ///< 小数点dp段，可与字符段码做按位或

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief TM1650 硬件抽象层接口
 * @details
 *  用户填充此结构体以适配目标平台。
 *  SCK 和 DIO 分别独立指定写回调，以支持不同GPIO端口。
 *  DIO 为双向引脚，额外需要读回调。
 */
typedef struct {
    tm1650_pin_write_t  pin_write_sck;   ///< SCK时钟线写回调
    tm1650_pin_write_t  pin_write_dio;   ///< DIO数据线写回调
    tm1650_pin_read_t   pin_read_dio;    ///< DIO数据线读回调
    tm1650_delay_us_t   delay_us;        ///< 微秒延时回调

    void               *user_data_sck;   ///< SCK引脚的用户实例指针
    void               *user_data_dio;   ///< DIO引脚的用户实例指针
} tm1650_hal_t;

/**
 * @brief TM1650 设备实例
 * @details
 *  持有硬件接口和当前配置状态。
 *  每个 TM1650 芯片对应一个实例，支持多实例。
 */
typedef struct {
    tm1650_hal_t  hal;                ///< 硬件抽象层接口
    uint8_t       system_config;      ///< 当前系统配置缓存
    bool          initialized;        ///< 初始化完成标志
} tm1650_t;

/*===========================================================================
 * 查表函数
 *===========================================================================*/

/**
 * @brief   获取数字字符（0-9）的7段码
 * @param   num  数字 0-9
 * @return  对应的7段段码；若越界返回 0x00（全灭）
 */
uint8_t tm1650_seg_for_digit(uint8_t num);

/**
 * @brief   获取十六进制字符（0-9, A-F）的7段码
 * @param   hex_val  值 0x0-0xF
 * @return  对应的7段段码；若越界返回 0x00（全灭）
 */
uint8_t tm1650_seg_for_hex(uint8_t hex_val);

/**
 * @brief   获取任意字符的近似7段码
 * @param   ch  字符（含数字、字母、'-'等）
 * @return  对应的段码，不支持则返回 0x00（全灭）
 */
uint8_t tm1650_seg_for_char(char ch);

/*===========================================================================
 * 初始化与配置
 *===========================================================================*/

/**
 * @brief   初始化 TM1650 设备实例
 * @param   self  TM1650 实例指针
 * @param   hal   硬件抽象层接口指针（其内容会被拷贝）
 * @return  true=成功, false=失败（参数为空）
 */
bool tm1650_init(tm1650_t *self, const tm1650_hal_t *hal);

/**
 * @brief   配置系统参数（亮度、段模式、工作模式、开关）
 * @param   self         TM1650 实例指针
 * @param   brightness   亮度等级，TM1650_LV1 ~ TM1650_LV8
 * @param   seg_mode     段模式，TM1650_8_SEGMENT_MODE 或 TM1650_7_SEGMENT_MODE
 * @param   work_mode    工作模式，TM1650_NORMAL_MODE 或 TM1650_STANDBY_MODE
 * @param   display_on   显示开关，TM1650_DISPLAY_ON 或 TM1650_DISPLAY_OFF
 * @return  true=配置成功, false=通信失败或参数为空
 */
bool tm1650_config(tm1650_t *self, uint8_t brightness,
                   uint8_t seg_mode, uint8_t work_mode, uint8_t display_on);

/**
 * @brief   快捷设置亮度
 * @param   self       TM1650 实例指针
 * @param   brightness 亮度等级，TM1650_LV1 ~ TM1650_LV8
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_set_brightness(tm1650_t *self, uint8_t brightness);

/**
 * @brief   打开显示
 * @param   self  TM1650 实例指针
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_display_on(tm1650_t *self);

/**
 * @brief   关闭显示
 * @param   self  TM1650 实例指针
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_display_off(tm1650_t *self);

/*===========================================================================
 * 低层显示操作（段码）
 *===========================================================================*/

/**
 * @brief   在指定位置显示原始段码
 * @param   self     TM1650 实例指针
 * @param   position 数码管位置，1-4（从左到右）
 * @param   seg_data 段码数据（0x00=全灭, 0xFF=全亮）
 * @return  true=成功, false=通信失败/参数为空/位置越界
 */
bool tm1650_display_segment(tm1650_t *self, uint8_t position, uint8_t seg_data);

/**
 * @brief   同时刷新全部4位数码管的段码
 * @param   self  TM1650 实例指针
 * @param   seg0  第1位数码管段码
 * @param   seg1  第2位数码管段码
 * @param   seg2  第3位数码管段码
 * @param   seg3  第4位数码管段码
 * @return  true=全部刷新成功, false=任一失败
 */
bool tm1650_display_segments(tm1650_t *self, uint8_t seg0, uint8_t seg1,
                             uint8_t seg2, uint8_t seg3);

/**
 * @brief   全屏清空（熄灭全部4位）
 * @param   self  TM1650 实例指针
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_clear(tm1650_t *self);

/*===========================================================================
 * 高层显示操作（自动译码）
 *===========================================================================*/

/**
 * @brief   整屏显示一个整数（0-9999）
 * @param   self    TM1650 实例指针
 * @param   number  要显示的整数，0-9999
 * @param   leading_zero  前导零处理：true=显示"0001", false=显示"   1"
 * @return  true=成功, false=通信失败或参数为空
 *
 * @note    超过 9999 的部分被截断，负数显示 "----"
 */
bool tm1650_display_integer(tm1650_t *self, int16_t number, bool leading_zero);

/**
 * @brief   整屏显示一个整数并用指定位指定小数点位置
 * @param   self    TM1650 实例指针
 * @param   number  要显示的整数（如 1234 配合 dot_pos=2 显示"12.34"）
 * @param   dot_pos  小数点在第几位数码管（1-4），0=不显示小数点
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_display_integer_with_dot(tm1650_t *self,
                                     int16_t number, uint8_t dot_pos);

/**
 * @brief   显示原始字符串（4个字符，自动转换段码）
 * @param   self  TM1650 实例指针
 * @param   str   以null结尾的字符串（最多取前4个字符）
 * @return  true=成功, false=通信失败或参数为空
 */
bool tm1650_display_string(tm1650_t *self, const char *str);

/**
 * @brief   逐段跑马灯测试
 * @param   self  TM1650 实例指针
 * @param   delay_ms  每步延时的毫秒数
 *
 * @note    此函数会阻塞，仅用于硬件调试
 */
void tm1650_test_segments(tm1650_t *self, uint16_t delay_ms);

/*===========================================================================
 * 按键扫描
 *===========================================================================*/

/**
 * @brief   读取按键状态
 * @param   self  TM1650 实例指针
 * @return  按键值（bit[3:0]对应KEY1-KEY4），0=无按键
 *
 * @note
 *   - 使用前需先完成 tm1650_init()
 *   - 对应模块上的4个微动按键
 *   - 返回后会自动恢复显示配置
 */
uint8_t tm1650_scan_key(tm1650_t *self);

/*===========================================================================
 * 已弃用的旧接口（兼容性保留，建议使用新命名）
 *===========================================================================
 *
 * 以下宏提供旧版API兼容映射，新项目请使用 tm1650_xxx 命名
 */

#define LV1                  TM1650_LV1
#define LV2                  TM1650_LV2
#define LV3                  TM1650_LV3
#define LV4                  TM1650_LV4
#define LV5                  TM1650_LV5
#define LV6                  TM1650_LV6
#define LV7                  TM1650_LV7
#define LV8                  TM1650_LV8
#define _8_SEGMENT_MODE      TM1650_8_SEGMENT_MODE
#define _7_SEGMENT_MODE      TM1650_7_SEGMENT_MODE
#define NORMAL_MODE          TM1650_NORMAL_MODE
#define STANDBY_MODE         TM1650_STANDBY_MODE
#define DISPLAY_ON           TM1650_DISPLAY_ON
#define DISPLAY_OFF          TM1650_DISPLAY_OFF
#define CMD_SYSTEM_CONFIG    TM1650_CMD_SYSTEM_CONFIG
#define CMD_READ_KEYPAD      TM1650_CMD_READ_KEYPAD
#define DIG1_ADDRESS         TM1650_DIG1_ADDRESS
#define DIG2_ADDRESS         TM1650_DIG2_ADDRESS
#define DIG3_ADDRESS         TM1650_DIG3_ADDRESS
#define DIG4_ADDRESS         TM1650_DIG4_ADDRESS
