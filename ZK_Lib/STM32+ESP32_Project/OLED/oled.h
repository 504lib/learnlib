#ifndef __OLED_H
#define __OLED_H 

#include "main.h"
#include "stdlib.h"	
#include "spi.h"

//-----------------OLED端口定义---------------- 

//#define OLED_SCL_Clr() HAL_GPIO_WritePin(SCL_OLED_GPIO_Port,SCL_OLED_Pin, GPIO_PIN_RESET)//SCL
//#define OLED_SCL_Set() HAL_GPIO_WritePin(SCL_OLED_GPIO_Port,SCL_OLED_Pin, GPIO_PIN_SET)

//#define OLED_SDA_Clr() HAL_GPIO_WritePin(SDA_OLED_GPIO_Port,SDA_OLED_Pin, GPIO_PIN_RESET)//SDA
//#define OLED_SDA_Set() HAL_GPIO_WritePin(SDA_OLED_GPIO_Port,SDA_OLED_Pin, GPIO_PIN_SET)

#define OLED_RES_Clr() HAL_GPIO_WritePin(RES_OLED_GPIO_Port,RES_OLED_Pin, GPIO_PIN_RESET)//RES
#define OLED_RES_Set() HAL_GPIO_WritePin(RES_OLED_GPIO_Port,RES_OLED_Pin, GPIO_PIN_SET)

#define OLED_DC_Clr()  HAL_GPIO_WritePin(DC_OLED_GPIO_Port,DC_OLED_Pin, GPIO_PIN_RESET)//DC
#define OLED_DC_Set()  HAL_GPIO_WritePin(DC_OLED_GPIO_Port,DC_OLED_Pin, GPIO_PIN_SET)
 		     
#define OLED_CS_Clr()  HAL_GPIO_WritePin(CS_OLED_GPIO_Port,CS_OLED_Pin, GPIO_PIN_RESET)//CS
#define OLED_CS_Set()  HAL_GPIO_WritePin(CS_OLED_GPIO_Port,CS_OLED_Pin, GPIO_PIN_SET)


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
	
void OLED_ClearPoint(u8 x,u8 y);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_WR_Byte(u8 dat,u8 mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode);
void OLED_DrawCircle(u8 x,u8 y,u8 r);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode);
void OLED_ShowChar6x8(u8 x,u8 y,u8 chr,u8 mode);
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode);
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode);
void OLED_ScrollDisplay(u8 num,u8 space,u8 mode);
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode);
void OLED_Init(void);

#endif

