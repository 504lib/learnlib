#ifndef __LCD_H
#define __LCD_H 
//#include "sys.h"
#include "stdlib.h"	
#include "main.h"
#include "spi.h"
#include "usart.h"
/*****************辰哥单片机设计******************
											STM32
 * 文件			:	TFT-LCD显示屏(1.8寸)h文件                     
 * 版本			: V1.0
 * 日期			: 2024.9.13
 * MCU			:	STM32F103C8T6
 * 接口			:	见代码							
 * BILIBILI	:	辰哥单片机设计
 * CSDN			:	辰哥单片机设计
 * 作者			:	辰哥 

**********************BEGIN***********************/

//----------------OLED端口定义----------------- 
/***************根据自己需求更改****************/
//#define LCD_SCL_GPIO_PORT				GPIOA
//#define LCD_SCL_GPIO_PIN				GPIO_Pin_0
//				
//#define LCD_SDA_GPIO_PORT				GPIOA
//#define LCD_SDA_GPIO_PIN				GPIO_Pin_1
//				
#define LCD_RST_GPIO_PORT				SPI1_RST_GPIO_Port
#define LCD_RST_GPIO_PIN				SPI1_RST_Pin
				
#define LCD_DC_GPIO_PORT				SPI1_DC_GPIO_Port
#define LCD_DC_GPIO_PIN					SPI1_DC_Pin
				
#define LCD_CS_GPIO_PORT				SPI1_CS_GPIO_Port
#define LCD_CS_GPIO_PIN					SPI1_CS_Pin

#define LCD_BLK_GPIO_PORT				SPI1_BLK_GPIO_Port
#define LCD_BLK_GPIO_PIN				SPI1_BLK_Pin

/*********************END**********************/


//#define LCD_SCLK_Clr() HAL_GPIO_WritePin(LCD_SCL_GPIO_PORT,LCD_SCL_GPIO_PIN)//SCL=SCLK
//#define LCD_SCLK_Set() GPIO_SetBits(LCD_SCL_GPIO_PORT,LCD_SCL_GPIO_PIN)

//#define LCD_MOSI_Clr() HAL_GPIO_WritePin(LCD_SDA_GPIO_PORT,LCD_SDA_GPIO_PIN)//SDA=MOSI
//#define LCD_MOSI_Set() GPIO_SetBits(LCD_SDA_GPIO_PORT,LCD_SDA_GPIO_PIN)

#define LCD_RES_Clr()  HAL_GPIO_WritePin(LCD_RST_GPIO_PORT,LCD_RST_GPIO_PIN,GPIO_PIN_RESET)//RES
#define LCD_RES_Set()  HAL_GPIO_WritePin(LCD_RST_GPIO_PORT,LCD_RST_GPIO_PIN,GPIO_PIN_SET)


#define LCD_DC_Clr()   HAL_GPIO_WritePin(LCD_DC_GPIO_PORT,LCD_DC_GPIO_PIN,GPIO_PIN_RESET)//DC
#define LCD_DC_Set()   HAL_GPIO_WritePin(LCD_DC_GPIO_PORT,LCD_DC_GPIO_PIN,GPIO_PIN_SET)
 		     
#define LCD_CS_Clr()   HAL_GPIO_WritePin(LCD_CS_GPIO_PORT,LCD_CS_GPIO_PIN,GPIO_PIN_RESET)//CS
#define LCD_CS_Set()   HAL_GPIO_WritePin(LCD_CS_GPIO_PORT,LCD_CS_GPIO_PIN,GPIO_PIN_SET)

#define LCD_BLK_Clr()  HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT,LCD_BLK_GPIO_PIN,GPIO_PIN_RESET)//BLK
#define LCD_BLK_Set()  HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT,LCD_BLK_GPIO_PIN,GPIO_PIN_SET)


#define USE_HORIZONTAL 1  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240
#define LCD_H 320

#else
#define LCD_W 320
#define LCD_H 240
#endif

//画笔颜色                        
#define WHITE                                    	 0xFFFF       	                             
#define BLACK                                     	 0x0000     	                         
#define BLUE                                       	 0x001F         	                         
#define BRED                                         0XF81F       	                         
#define GRED 	                                     0XFFE0       	                         
#define GBLUE	                                     0X07FF       	                         
#define RED                                       	 0xF800       	                         
#define MAGENTA                                   	 0xF81F       	                         
#define GREEN                                     	 0x07E0       	                         
#define CYAN                                      	 0x7FFF       	                         
#define YELLOW                                    	 0xFFE0       	                         
#define BROWN			                             0XBC40 //棕色       	                         
#define BRRED 		                                 0XFC07 //棕红色       	                         
#define GRAY  		                                 0X8430 //灰色       	                         
#define DARKBLUE                                  	 0X01CF	//深蓝色       	                         
#define LIGHTBLUE                                  	 0X7D7C	//浅蓝色         	                         
#define GRAYBLUE                                   	 0X5458 //灰蓝色       	                         
#define LIGHTGREEN                                 	 0X841F //浅绿色       	                         
#define LGRAY						 			     0XC618 //浅灰色(PANNEL),窗体背景色       	                         
#define LGRAYBLUE                                    0XA651 //浅灰蓝色(中间层颜色)       	                         
#define LBBLUE                                       0X2B12 //浅棕蓝色(选择条目的反色)       	                         

// void LCD_GPIO_Init(void);//初始化GPIO
void LCD_Writ_Bus(uint8_t dat);//模拟SPI时序
void LCD_WR_DATA8(uint8_t dat);//写入一个字节
void LCD_WR_DATA(uint16_t dat);//写入两个字节
void LCD_WR_REG(uint8_t dat);//写入一个指令
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);//设置坐标函数
void LCD_Init(void);//LCD初始化


void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);//指定区域填充颜色
void LCD_Fill_DMA_DoubleBuffer(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color);//在指定位置画一个点
void LCD_DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);//在指定位置画一条线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color);//在指定位置画一个矩形
void Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color);//在指定位置画一个圆

void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示汉字串
void LCD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个12x12汉字
void LCD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个16x16汉字
void LCD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个24x24汉字
void LCD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示单个32x32汉字

void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示一个字符
void LCD_ShowChar_DMA(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);
void LCD_ShowString(uint16_t x,uint16_t y,const uint8_t *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);//显示字符串 uint32_t mypow(uint8_t m,uint8_t n);//求幂 void LCD_ShowIntNum(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey);//显示整数变量
void LCD_ShowString_DMA(uint16_t x,uint16_t y,const uint8_t *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);
void LCD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey);//显示两位小数变量

void LCD_ShowPicture(uint16_t x,uint16_t y,uint16_t length,uint16_t width,const uint8_t pic[]);//显示图片


#endif

