//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//测试硬件：单片机STM32H743IIT6,正点原子Apollo STM32F4/F7开发板,主频400MHZ，晶振25MHZ
//QDtech-TFT液晶驱动 for STM32 IO模拟
//xiao冯@ShenZhen QDtech co.,LTD
//公司网站:www.qdtft.com
//淘宝网站：http://qdtech.taobao.com
//wiki技术网站：http://www.lcdwiki.com
//我司提供技术支持，任何技术问题欢迎随时交流学习
//固话(传真) :+86 0755-23594567 
//手机:15989313508（冯工） 
//邮箱:lcdwiki01@gmail.com    support@lcdwiki.com    goodtft@163.com 
//技术支持QQ:3002773612  3002778157
//技术交流QQ群:324828016
//创建日期:2018/08/22
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 深圳市全动电子技术有限公司 2018-2028
//All rights reserved
/****************************************************************************************************
//=========================================电源接线================================================//
//     LCD模块                STM32单片机
//      VCC          接        DC5V/3.3V      //电源
//      GND          接          GND          //电源地
//=======================================液晶屏数据线接线==========================================//
//本模块默认数据总线类型为SPI总线
//     LCD模块                STM32单片机    
//    SDI(MOSI)      接          PB15         //液晶屏SPI总线数据写信号
//    SDO(MISO)      接          PB14         //液晶屏SPI总线数据读信号，如果不需要读，可以不接线
//=======================================液晶屏控制线接线==========================================//
//     LCD模块 					      STM32单片机 
//       LED         接          PD6          //液晶屏背光控制信号，如果不需要控制，接5V或3.3V
//       SCK         接          PB13         //液晶屏SPI总线时钟信号
//     LCD_RS        接          PD5          //液晶屏数据/命令控制信号
//     LCD_RST       接          PD12         //液晶屏复位控制信号
//     LCD_CS        接          PD11         //液晶屏片选控制信号
//=========================================触摸屏触接线=========================================//
//如果模块不带触摸功能或者带有触摸功能，但是不需要触摸功能，则不需要进行触摸屏接线
//	   LCD模块                STM32单片机 
//     CTP_INT       接          PH11         //电容触摸屏中断信号
//     CTP_SDA       接          PI3          //电容触摸屏IIC数据信号
//     CTP_RST       接          PI8          //电容触摸屏复位信号
//     CTP_SCL       接          PH6          //电容触摸屏IIC时钟信号
**************************************************************************************************/	
 /* @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**************************************************************************************************/	
#include "lcd.h"
#include "stdlib.h"
#include "spi.h"
#include "main.h"

	   
//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;

//画笔颜色,背景颜色
uint16_t POINT_COLOR = 0x0000,BACK_COLOR = 0xFFFF;  
uint16_t DeviceCode;	 

uint16_t dma_buffer1[LCD_W * 32];	//LCD缓存区
uint16_t dma_buffer2[LCD_W * 32];	//LCD缓存区

/*****************************************************************************
 * @name       :void LCD_WR_REG(uint8_t data)
 * @date       :2018-08-09 
 * @function   :Write an 8-bit command to the LCD screen
 * @parameters :data:Command value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_REG(uint8_t data)
{ 
	LCD_CS_CLR;     
	LCD_RS_CLR;	  
    HAL_SPI_Transmit_DMA(&hspi1, &data,1); 
    while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	LCD_CS_SET;	
}

/*****************************************************************************
 * @name       :void LCD_WR_DATA(uint8_t data)
 * @date       :2018-08-09 
 * @function   :Write an 8-bit data to the LCD screen
 * @parameters :data:data value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_DATA(uint8_t data)
{
	LCD_CS_CLR;
	LCD_RS_SET;
    HAL_SPI_Transmit_DMA(&hspi1, &data,1); 
    while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	LCD_CS_SET;
}

uint8_t LCD_RD_DATA(void)
{
	 uint8_t data = 0;
	 LCD_CS_CLR;
	 LCD_RS_SET;
	//  SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_32);
	//  data = SPI2_ReadWriteByte(0xFF);
	//  SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_8);
	 LCD_CS_SET;
	 return data;
}

/*****************************************************************************
 * @name       :void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
 * @date       :2018-08-09 
 * @function   :Write data into registers
 * @parameters :LCD_Reg:Register address
                LCD_RegValue:Data to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{	
	LCD_WR_REG(LCD_Reg);  
	LCD_WR_DATA(LCD_RegValue);	    		 
}	   

uint8_t LCD_ReadReg(uint8_t LCD_Reg)
{
	LCD_WR_REG(LCD_Reg);
	return LCD_RD_DATA();
}

/*****************************************************************************
 * @name       :void LCD_WriteRAM_Prepare(void)
 * @date       :2018-08-09 
 * @function   :Write GRAM
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	 
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}	 


void LCD_ReadRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.rramcmd);
}	 

/*****************************************************************************
 * @name       :void Lcd_WriteData_16Bit(uint16 Data)
 * @date       :2018-08-09 
 * @function   :Write an 16-bit command to the LCD screen
 * @parameters :Data:Data to be written
 * @retvalue   :None
******************************************************************************/	 
void Lcd_WriteData_16Bit(uint16_t Data)
{
	uint8_t data_low = Data & 0xFF;
	uint8_t data_high = Data >> 8;
	LCD_CS_CLR;
	LCD_RS_SET;  
    HAL_SPI_Transmit_DMA(&hspi1, &data_high,1); 
    HAL_SPI_Transmit_DMA(&hspi1, &data_low,1); 
    while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	LCD_CS_SET;
}

uint16_t Color_To_565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

uint16_t Lcd_ReadData_16Bit(void)
{
	uint8_t r,g,b;
	LCD_RS_SET;
	// LCD_CS_CLR;
	// SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_32);
	// //dummy data
	//  SPI2_ReadWriteByte(0xFF);
	// //8bit:red data
	// //16bit:red and green data
	// r= SPI2_ReadWriteByte(0xFF);
	
	// //8bit:green data
	// //16bit:blue data
	// g= SPI2_ReadWriteByte(0xFF);
	
	// //blue data
	// b= SPI2_ReadWriteByte(0xFF);
	// //r >>= 8;
	// //g >>= 8;
	// //b >>= 8;
	// SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_8);
	LCD_CS_SET;
	return Color_To_565(r, g, b);
}

/*****************************************************************************
 * @name       :void LCD_DrawPoint(uint16_t x,uint16_t y)
 * @date       :2018-08-09 
 * @function   :Write a pixel data at a specified location
 * @parameters :x:the x coordinate of the pixel
                y:the y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/	
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
	LCD_SetCursor(x,y);//设置光标位置 
	Lcd_WriteData_16Bit(POINT_COLOR); 
}

uint16_t LCD_ReadPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);//设置光标位置 
	LCD_ReadRAM_Prepare();
	return Lcd_ReadData_16Bit();
}

/*****************************************************************************
 * @name       :void LCD_Clear(uint16_t Color)
 * @date       :2018-08-09 
 * @function   :Full screen filled LCD screen
 * @parameters :color:Filled color
 * @retvalue   :None
******************************************************************************/	
void LCD_Clear(uint16_t Color)
{
	static uint8_t current_buffer = 0;
	uint8_t color_high = Color >> 8;
	uint8_t color_low = Color & 0xFF;
	unsigned int i,m;  
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);   
	LCD_CS_CLR;
	LCD_RS_SET;
	for (uint32_t i = 0; i < LCD_W * 32; i++)
	{
		dma_buffer1[i] = Color;
		dma_buffer2[i] = Color;
	}
	
    for(uint32_t y = 0; y < LCD_H / 32; y++) 
	{
        // 等待前一次DMA完成
        if(y > 0) 
		{
            while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
        }
        
        // 启动下一次DMA
        if(current_buffer == 0) 
		{
            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buffer1, LCD_W * 64);
            current_buffer = 1;
        } 
		else 
		{
            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buffer2, LCD_W * 64);
            current_buffer = 0;
        }
    }
	while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
	LCD_CS_SET;
} 
//void LCD_Fill_DMA_DoubleBuffer(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
//{
////    uint32_t width = xend - xsta;
//    uint32_t height = yend - ysta;
//    
//    LCD_Address_Set(xsta, ysta, xend-1, yend-1);
//    LCD_CS_Clr();
//    LCD_DC_Set();
//    
//    // 填充缓冲区
//    for(int i = 0; i < 128; i++) {
//        dma_buffer1[i] = color;
//        dma_buffer2[i] = color;
//    }
//    
//    for(uint32_t y = 0; y < height; y++) 
//	{
//        // 等待前一次DMA完成
//        if(y > 0) 
//		{
//            while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
//        }
//        
//        // 启动下一次DMA
//        if(current_buffer == 0) 
//		{
//            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buffer1, 128 * 2);
//            current_buffer = 1;
//        } 
//		else 
//		{
//            HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buffer2, 128 * 2);
//            current_buffer = 0;
//        }
//    }
//    
//    // 等待最后一次传输完成
//    while(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
//    LCD_CS_Set();
//}

/*****************************************************************************
 * @name       :void LCD_Clear(uint16_t Color)
 * @date       :2018-08-09 
 * @function   :Initialization LCD screen GPIO
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	
void LCD_GPIOInit(void)
{
		    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOD_CLK_ENABLE();           //使能GPIOF时钟
    
    //PF6
    GPIO_Initure.Pin=GPIO_PIN_5| GPIO_PIN_6|GPIO_PIN_11| GPIO_PIN_12;            //PF6
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;     //快速         
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);     //初始化
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_5| GPIO_PIN_6|GPIO_PIN_11|GPIO_PIN_12,GPIO_PIN_SET);
		LCD_LED(1);
}

/*****************************************************************************
 * @name       :void LCD_RESET(void)
 * @date       :2018-08-09 
 * @function   :Reset LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	
void LCD_RESET(void)
{
	LCD_RST_SET;
	HAL_Delay(50);
	LCD_RST_CLR;
	HAL_Delay(100);	
	LCD_RST_SET;
	HAL_Delay(50);
}

/*****************************************************************************
 * @name       :void LCD_RESET(void)
 * @date       :2018-08-09 
 * @function   :Initialization LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	 	 
void LCD_Init(void)
{  
 	LCD_RESET(); //LCD 复位
//*************2.8 ILI9341 IPS初始化**********//	
	LCD_WR_REG(0xCF);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0xC1); 
	LCD_WR_DATA(0x30); 
 
	LCD_WR_REG(0xED);  
	LCD_WR_DATA(0x64); 
	LCD_WR_DATA(0x03); 
	LCD_WR_DATA(0X12); 
	LCD_WR_DATA(0X81); 
 
	LCD_WR_REG(0xE8);  
	LCD_WR_DATA(0x85); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x78); 

	LCD_WR_REG(0xCB);  
	LCD_WR_DATA(0x39); 
	LCD_WR_DATA(0x2C); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x34); 
	LCD_WR_DATA(0x02); 
	
	LCD_WR_REG(0xF7);  
	LCD_WR_DATA(0x20); 
 
	LCD_WR_REG(0xEA);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 

	LCD_WR_REG(0xC0);       //Power control 
	LCD_WR_DATA(0x13);     //VRH[5:0] 
 
	LCD_WR_REG(0xC1);       //Power control 
	LCD_WR_DATA(0x13);     //SAP[2:0];BT[3:0] 
 
	LCD_WR_REG(0xC5);       //VCM control 
	LCD_WR_DATA(0x22);   //22
	LCD_WR_DATA(0x35);   //35
 
	LCD_WR_REG(0xC7);       //VCM control2 
	LCD_WR_DATA(0xBD);  //AF

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x36);       // Memory Access Control 
	LCD_WR_DATA(0x08); 

	LCD_WR_REG(0xB6);  
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0xA2); 

	LCD_WR_REG(0x3A);       
	LCD_WR_DATA(0x55); 

	LCD_WR_REG(0xF6);  //Interface Control
	LCD_WR_DATA(0x01); 
	LCD_WR_DATA(0x30);  //MCU

	LCD_WR_REG(0xB1);       //VCM control 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x1B); 
 
	LCD_WR_REG(0xF2);       // 3Gamma Function Disable 
	LCD_WR_DATA(0x00); 
 
	LCD_WR_REG(0x26);       //Gamma curve selected 
	LCD_WR_DATA(0x01); 
 
	LCD_WR_REG(0xE0);       //Set Gamma 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x35); 
	LCD_WR_DATA(0x31); 
	LCD_WR_DATA(0x0B); 
	LCD_WR_DATA(0x0E); 
	LCD_WR_DATA(0x06); 
	LCD_WR_DATA(0x49); 
	LCD_WR_DATA(0xA7); 
	LCD_WR_DATA(0x33); 
	LCD_WR_DATA(0x07); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x03); 
	LCD_WR_DATA(0x0C); 
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0x00); 
 
	LCD_WR_REG(0XE1);       //Set Gamma 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x04); 
	LCD_WR_DATA(0x11); 
	LCD_WR_DATA(0x08); 
	LCD_WR_DATA(0x36); 
	LCD_WR_DATA(0x58); 
	LCD_WR_DATA(0x4D); 
	LCD_WR_DATA(0x07); 
	LCD_WR_DATA(0x10); 
	LCD_WR_DATA(0x0C); 
	LCD_WR_DATA(0x32); 
	LCD_WR_DATA(0x34); 
	LCD_WR_DATA(0x0F); 

	LCD_WR_REG(0x11);       //Exit Sleep 
	HAL_Delay(120); 
	LCD_WR_REG(0x29);       //Display on 

	LCD_LED(1);
	LCD_direction(USE_HORIZONTAL);//设置LCD显示方向 
	LCD_Clear(WHITE);//清全屏白色
}
 
/*****************************************************************************
 * @name       :void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
 * @date       :2018-08-09 
 * @function   :Setting LCD display window
 * @parameters :xStar:the bebinning x coordinate of the LCD display window
								yStar:the bebinning y coordinate of the LCD display window
								xEnd:the endning x coordinate of the LCD display window
								yEnd:the endning y coordinate of the LCD display window
 * @retvalue   :None
******************************************************************************/ 
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{	
	LCD_WR_REG(lcddev.setxcmd);	
	LCD_WR_DATA(xStar>>8);
	LCD_WR_DATA(0x00FF&xStar);		
	LCD_WR_DATA(xEnd>>8);
	LCD_WR_DATA(0x00FF&xEnd);

	LCD_WR_REG(lcddev.setycmd);	
	LCD_WR_DATA(yStar>>8);
	LCD_WR_DATA(0x00FF&yStar);		
	LCD_WR_DATA(yEnd>>8);
	LCD_WR_DATA(0x00FF&yEnd);

	LCD_WriteRAM_Prepare();	//开始写入GRAM			
}   

/*****************************************************************************
 * @name       :void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
 * @date       :2018-08-09 
 * @function   :Set coordinate value
 * @parameters :Xpos:the  x coordinate of the pixel
								Ypos:the  y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/ 
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	  	    			
	LCD_SetWindows(Xpos,Ypos,Xpos,Ypos);	
} 

/*****************************************************************************
 * @name       :void LCD_direction(uint8_t direction)
 * @date       :2018-08-09 
 * @function   :Setting the display direction of LCD screen
 * @parameters :direction:0-0 degree
                          1-90 degree
													2-180 degree
													3-270 degree
 * @retvalue   :None
******************************************************************************/ 
void LCD_direction(uint8_t direction)
{ 
	lcddev.setxcmd=0x2A;
	lcddev.setycmd=0x2B;
	lcddev.wramcmd=0x2C;
	lcddev.rramcmd=0x2E;
	lcddev.dir = direction%4;
	switch(lcddev.dir)
	{		  
		case 0:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;		
			LCD_WriteReg(0x36,(1<<3)|(0<<6)|(0<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 1:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(0<<7)|(1<<6)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;
		case 2:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;	
			LCD_WriteReg(0x36,(1<<3)|(1<<6)|(1<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 3:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<7)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;	
		default:break;
	}		
}	 

uint16_t LCD_Read_ID(void)
{
uint8_t i,val[3] = {0};
	for(i=1;i<4;i++)
	{
		LCD_WR_REG(0xD9);
		LCD_WR_DATA(0x10+i);
		LCD_WR_REG(0xD3);
		val[i-1] = LCD_RD_DATA();
	}
	lcddev.id=val[1];
	lcddev.id<<=8;
	lcddev.id|=val[2];
	return lcddev.id;
}
