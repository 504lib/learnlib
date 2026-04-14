/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>
#include "../SPI_OLED/oled.h"
#include "../DL_GetTick_Folder/DL_Tick.h"
#include "../multikey/multikey.h"

volatile unsigned char uart_data = 0;
char buffuer[256] = {0};
MulitKey_t key;

void uart0_send_char(char ch); //串口0发送单个字符
void uart0_send_string(char* str); //串口0发送字符串

uint8_t Key1ReadPinCallback(MulitKey_t* key)
{
    return DL_GPIO_readPins(KEY_GROIP_PORT,KEY_GROIP_BOARD_KEY_PIN);
}

void Key1PressdCallback(MulitKey_t* key)
{
    DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
}

int fputc(int ch, FILE *f)
{
    // 关键：使用阻塞式发送函数，直到字符成功发送
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)ch);
    return ch;
}

int fputs(const char *_ptr, FILE *_fp)
{
    uint16_t i, len;
    len = strlen(_ptr);
    for (i = 0; i < len; i++)
    {
        // 复用 fputc 逐个字符发送
        fputc(_ptr[i], _fp);
    }
    return len;
}

int puts(const char *_ptr)
{
    int count;
    // 先发送字符串本身
    count = fputs(_ptr, stdout);
    // 再发送一个换行符
    count += fputs("\n", stdout);
    return count;
}

int main(void)
{
    SYSCFG_DL_init();
    //清除串口中断标志
    // NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    //使能串口中断
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    OLED_Init();
    OLED_Clear();
    OLED_DisPlay_On();
    OLED_ShowString(0, 0, (uint8_t*)"test", 16, 1);
    OLED_Refresh();
    volatile uint32_t lasttick = 0;
    MulitKey_Init(&key,Key1ReadPinCallback,Key1PressdCallback,NULL,FALL_BORDER_TRIGGER);
    // volatile uint32_t cnt = 0;
    uart0_send_string("uart0 start work\r\n");
    printf("测试\n");
    while (1)
    {
        MulitKey_Scan(&key);
        if(DL_GetTick() - lasttick >= 20){
            uint32_t current_tick = DL_GetTick();
            uint32_t dt = current_tick - lasttick;
            snprintf(buffuer, sizeof(buffuer), "dt:%u\n",dt);
            
            OLED_ShowString(0, 0, (uint8_t*)buffuer, 16, 1);
            printf("%*s",sizeof(buffuer),buffuer);
            OLED_Refresh();  
            lasttick = DL_GetTick();         //    cnt++;
        }
        // printf("The tick is: %d\r\n",tick);
    }
}

//串口发送单个字符
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_0_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_0_INST, ch);
}
//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}

//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_0_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
            //将发送过来的数据保存在变量中
            uart_data = DL_UART_Main_receiveData(UART_0_INST);
            //将保存的数据再发送出去
            // uart0_send_char(uart_data);
            printf("receive data:%c\n",uart_data);
            break;

        default://其他的串口中断
            break;
    }
}


