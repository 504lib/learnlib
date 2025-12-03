/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "lcd.h"
#include "spi.h"
#include <stdio.h>
/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    // static lv_disp_draw_buf_t draw_buf_dsc_1;
    // static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 2) */
   static lv_disp_draw_buf_t draw_buf_dsc_2;
   static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
   static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
   lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

//    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
//    static lv_disp_draw_buf_t draw_buf_dsc_3;
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
//                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static lv_disp_drv_t *disp_drv_for_callback = NULL;
static const lv_area_t *flush_area = NULL;

volatile uint8_t lcd_dma_complete = 1;  // DMA完成标志

// 等待DMA完成
void LCD_WaitForDMA(void)
{
    uint32_t timeout = 100000;  // 超时计数
    
    while(lcd_dma_complete == 0 && timeout > 0) {
        timeout--;
    }
    
    if(timeout == 0) {
        printf("DMA timeout!\n");
    }
}

// DMA传输完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi->Instance == SPI1) {
        lcd_dma_complete = 1;
    }
}

// 逐行传输函数（最稳定的版本）
void LCD_WriteBuffer_DMA_LineByLine(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, lv_color_t *color_buffer)
{
    uint16_t width = xend - xsta + 1;
    uint16_t height = yend - ysta + 1;
    
    // 设置窗口（整个区域）
    LCD_SetWindows(xsta, ysta, xend, yend);
    
    // 注意：这里我们不保持CS为低，而是每行都重新拉低CS
    // 这样更符合ILI9341的要求
    
    // 逐行传输
    for(uint16_t y = 0; y < height; y++) {
        // 等待上一次DMA完成
        LCD_WaitForDMA();
        
        // 设置DMA完成标志为0
        lcd_dma_complete = 0;
        
        // 拉低CS并设置RS
        LCD_CS_CLR;
        LCD_RS_SET;
        
        // 为当前行准备缓冲区（静态或动态）
        static uint16_t line_buffer[320];  // 假设最大宽度为320
        
        // 转换当前行的颜色数据
        for(uint16_t x = 0; x < width; x++) {
            lv_color_t lv_color = color_buffer[y * width + x];
            
            // 调试：输出第一行的第一个像素
            if(y == 0 && x == 0) {
                printf("First pixel: R=%d, G=%d, B=%d, full=0x%08X\n", 
                       lv_color.ch.red, lv_color.ch.green, lv_color.ch.blue, lv_color.full);
            }
            
            // 颜色转换：根据LVGL配置选择正确的转换方式
            #if LV_COLOR_DEPTH == 16
                // LVGL已经是16位颜色
                uint16_t color = lv_color.full;
                
                // 注意：可能需要字节交换
                #if LV_COLOR_16_SWAP == 1
                    color = (color >> 8) | (color << 8);
                #endif
                
            #elif LV_COLOR_DEPTH == 32
                // 从ARGB8888转换为RGB565
                uint8_t r = (lv_color.ch.red >> 3) & 0x1F;
                uint8_t g = (lv_color.ch.green >> 2) & 0x3F;
                uint8_t b = (lv_color.ch.blue >> 3) & 0x1F;
                uint16_t color = (r << 11) | (g << 5) | b;
                
            #else
                // 默认处理
                uint16_t color = lv_color.full;
            #endif
            
            line_buffer[x] = color;
        }
        
        // 启动DMA传输当前行
        HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)line_buffer, width * 2);
        
        // 等待DMA完成（同步方式，更稳定）
        LCD_WaitForDMA();
        
        // 拉高CS
        LCD_CS_SET;
        
        // 延迟一小段时间（可选，某些屏幕需要）
        // for(int i = 0; i < 10; i++);
    }
}


/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

    if(disp_flush_enabled) {
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
#if 0
        // disp_drv_for_callback = disp_drv;
        // flush_area = area;

        LCD_WriteBuffer_DMA_LineByLine(area->x1, area->y1, area->x2, area->y2, color_p);
        // 启动异步DMA传输
        // LCD_WriteBuffer_DMA_Async(area->x1, area->y1, area->x2, area->y2, color_p);
	}
#else
		uint16_t width = area->x2 - area->x1 + 1;
        uint16_t height = area->y2 - area->y1 + 1;
        
        // 设置窗口
        LCD_SetWindows(area->x1, area->y1, area->x2, area->y2);
        
        // 批量传输数据
        LCD_CS_CLR;
        LCD_RS_SET;
        
        // 一次传输一行数据
        for(uint16_t y = 0; y < height; y++) {
            // 传输一行数据
            for(uint16_t x = 0; x < width; x++) {
                uint16_t color = color_p->full;
                
                // 使用直接SPI传输而不是函数调用
                uint8_t data_buffer[2];
                data_buffer[0] = color >> 8;
                data_buffer[1] = color & 0xFF;
                HAL_SPI_Transmit(&hspi1, data_buffer, 2,1);
//	            while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
                color_p++;
            }
        }
        
        LCD_CS_SET;
    }
        
    /*IMPORTANT!!!	
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
#endif		

    

}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
