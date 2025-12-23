/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdio.h>
// #include "core/lv_event.h"
// #include "core/lv_obj.h"
#include "lvgl.h"
#include "main.h"
#include "stm32h7xx_hal_gpio.h"
// #include "widgets/lv_label.h"
#include "custom.h"
#include <stdbool.h>
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**
 * Create a demo application
 */

static void btn_clicked_event_cb(lv_event_t * e)
{
    static uint32_t count = 0;
    lv_ui* ui = (lv_ui*)lv_event_get_user_data(e);
    if(!ui) return;
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        printf("count: %lu\r\n",(unsigned long)++count);
        lv_label_set_text_fmt(ui->screen_btn_1_label, "count:%lu", (unsigned long)count);
    }
}

static void toggle_led_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        lv_ui* ui = (lv_ui*)lv_event_get_user_data(e);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, lv_obj_has_state(ui->screen_LED_TOGGLE, LV_STATE_CHECKED) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        lv_label_set_text_fmt(ui->screen_LED_label, "LED is %s", lv_obj_has_state(ui->screen_LED_TOGGLE, LV_STATE_CHECKED) ? "ON" : "OFF");
    }
}

void custom_init(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_btn_1, btn_clicked_event_cb, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_LED_TOGGLE, toggle_led_event_cb,LV_EVENT_ALL , ui);
}

