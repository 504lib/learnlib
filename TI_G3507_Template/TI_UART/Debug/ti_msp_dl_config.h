/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMG0
#define PWM_0_INST_IRQHandler                                   TIMG0_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMG0_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                         DL_GPIO_PIN_12
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM34)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM34_PF_TIMG0_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                         DL_GPIO_PIN_13
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM35)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA0)
#define TIMER_0_INST_IRQHandler                                 TIMA0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                            (39U)
/* Defines for TIMER_1 */
#define TIMER_1_INST                                                     (TIMA1)
#define TIMER_1_INST_IRQHandler                                 TIMA1_IRQHandler
#define TIMER_1_INST_INT_IRQN                                   (TIMA1_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                           (799U)




/* Defines for I2C_6050 */
#define I2C_6050_INST                                                       I2C0
#define I2C_6050_INST_IRQHandler                                 I2C0_IRQHandler
#define I2C_6050_INST_INT_IRQN                                     I2C0_INT_IRQn
#define GPIO_I2C_6050_SDA_PORT                                             GPIOA
#define GPIO_I2C_6050_SDA_PIN                                      DL_GPIO_PIN_0
#define GPIO_I2C_6050_IOMUX_SDA                                   (IOMUX_PINCM1)
#define GPIO_I2C_6050_IOMUX_SDA_FUNC                    IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_6050_SCL_PORT                                             GPIOA
#define GPIO_I2C_6050_SCL_PIN                                      DL_GPIO_PIN_1
#define GPIO_I2C_6050_IOMUX_SCL                                   (IOMUX_PINCM2)
#define GPIO_I2C_6050_IOMUX_SCL_FUNC                    IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                            4000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_4_MHZ_115200_BAUD                                        (2)
#define UART_0_FBRD_4_MHZ_115200_BAUD                                       (11)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART2
#define UART_1_INST_FREQUENCY                                           32000000
#define UART_1_INST_IRQHandler                                  UART2_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                        DL_GPIO_PIN_22
#define GPIO_UART_1_TX_PIN                                        DL_GPIO_PIN_21
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM47)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM46)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM47_PF_UART2_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM46_PF_UART2_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_1_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_2 */
#define UART_2_INST                                                        UART1
#define UART_2_INST_FREQUENCY                                           32000000
#define UART_2_INST_IRQHandler                                  UART1_IRQHandler
#define UART_2_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_2_RX_PORT                                                GPIOA
#define GPIO_UART_2_TX_PORT                                                GPIOA
#define GPIO_UART_2_RX_PIN                                        DL_GPIO_PIN_18
#define GPIO_UART_2_TX_PIN                                        DL_GPIO_PIN_17
#define GPIO_UART_2_IOMUX_RX                                     (IOMUX_PINCM40)
#define GPIO_UART_2_IOMUX_TX                                     (IOMUX_PINCM39)
#define GPIO_UART_2_IOMUX_RX_FUNC                      IOMUX_PINCM40_PF_UART1_RX
#define GPIO_UART_2_IOMUX_TX_FUNC                      IOMUX_PINCM39_PF_UART1_TX
#define UART_2_BAUD_RATE                                                (115200)
#define UART_2_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_2_FBRD_32_MHZ_115200_BAUD                                      (23)




/* Defines for SPI */
#define SPI_INST                                                           SPI0
#define SPI_INST_IRQHandler                                     SPI0_IRQHandler
#define SPI_INST_INT_IRQN                                         SPI0_INT_IRQn
#define GPIO_SPI_PICO_PORT                                                GPIOA
#define GPIO_SPI_PICO_PIN                                         DL_GPIO_PIN_9
#define GPIO_SPI_IOMUX_PICO                                     (IOMUX_PINCM20)
#define GPIO_SPI_IOMUX_PICO_FUNC                     IOMUX_PINCM20_PF_SPI0_PICO
#define GPIO_SPI_POCI_PORT                                                GPIOA
#define GPIO_SPI_POCI_PIN                                         DL_GPIO_PIN_4
#define GPIO_SPI_IOMUX_POCI                                      (IOMUX_PINCM9)
#define GPIO_SPI_IOMUX_POCI_FUNC                      IOMUX_PINCM9_PF_SPI0_POCI
/* GPIO configuration for SPI */
#define GPIO_SPI_SCLK_PORT                                                GPIOB
#define GPIO_SPI_SCLK_PIN                                        DL_GPIO_PIN_18
#define GPIO_SPI_IOMUX_SCLK                                     (IOMUX_PINCM44)
#define GPIO_SPI_IOMUX_SCLK_FUNC                     IOMUX_PINCM44_PF_SPI0_SCLK



/* Port definition for Pin Group BUZZER */
#define BUZZER_PORT                                                      (GPIOA)

/* Defines for Buzzer: GPIOA.14 with pinCMx 36 on package pin 7 */
#define BUZZER_Buzzer_PIN                                       (DL_GPIO_PIN_14)
#define BUZZER_Buzzer_IOMUX                                      (IOMUX_PINCM36)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for LED_1: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED_LED_1_PIN                                           (DL_GPIO_PIN_22)
#define LED_LED_1_IOMUX                                          (IOMUX_PINCM50)
/* Defines for LED_2: GPIOB.17 with pinCMx 43 on package pin 14 */
#define LED_LED_2_PIN                                           (DL_GPIO_PIN_17)
#define LED_LED_2_IOMUX                                          (IOMUX_PINCM43)
/* Defines for LED_3: GPIOB.16 with pinCMx 33 on package pin 4 */
#define LED_LED_3_PIN                                           (DL_GPIO_PIN_16)
#define LED_LED_3_IOMUX                                          (IOMUX_PINCM33)
/* Port definition for Pin Group SPI_OLED */
#define SPI_OLED_PORT                                                    (GPIOB)

/* Defines for OLED_RES: GPIOB.10 with pinCMx 27 on package pin 62 */
#define SPI_OLED_OLED_RES_PIN                                   (DL_GPIO_PIN_10)
#define SPI_OLED_OLED_RES_IOMUX                                  (IOMUX_PINCM27)
/* Defines for OLED_DC: GPIOB.11 with pinCMx 28 on package pin 63 */
#define SPI_OLED_OLED_DC_PIN                                    (DL_GPIO_PIN_11)
#define SPI_OLED_OLED_DC_IOMUX                                   (IOMUX_PINCM28)
/* Defines for OLED_CS: GPIOB.14 with pinCMx 31 on package pin 2 */
#define SPI_OLED_OLED_CS_PIN                                    (DL_GPIO_PIN_14)
#define SPI_OLED_OLED_CS_IOMUX                                   (IOMUX_PINCM31)
/* Port definition for Pin Group KEY_GROIP */
#define KEY_GROIP_PORT                                                   (GPIOB)

/* Defines for BOARD_KEY: GPIOB.21 with pinCMx 49 on package pin 20 */
#define KEY_GROIP_BOARD_KEY_PIN                                 (DL_GPIO_PIN_21)
#define KEY_GROIP_BOARD_KEY_IOMUX                                (IOMUX_PINCM49)
/* Defines for KEY1: GPIOB.15 with pinCMx 32 on package pin 3 */
#define KEY_GROIP_KEY1_PIN                                      (DL_GPIO_PIN_15)
#define KEY_GROIP_KEY1_IOMUX                                     (IOMUX_PINCM32)
/* Defines for KEY2: GPIOB.13 with pinCMx 30 on package pin 1 */
#define KEY_GROIP_KEY2_PIN                                      (DL_GPIO_PIN_13)
#define KEY_GROIP_KEY2_IOMUX                                     (IOMUX_PINCM30)
/* Defines for KEY3: GPIOB.26 with pinCMx 57 on package pin 28 */
#define KEY_GROIP_KEY3_PIN                                      (DL_GPIO_PIN_26)
#define KEY_GROIP_KEY3_IOMUX                                     (IOMUX_PINCM57)
/* Defines for KEY4: GPIOB.27 with pinCMx 58 on package pin 29 */
#define KEY_GROIP_KEY4_PIN                                      (DL_GPIO_PIN_27)
#define KEY_GROIP_KEY4_IOMUX                                     (IOMUX_PINCM58)
/* Port definition for Pin Group ENCODER */
#define ENCODER_PORT                                                     (GPIOA)

/* Defines for E1A: GPIOA.28 with pinCMx 3 on package pin 35 */
// pins affected by this interrupt request:["E1A","E1B","E2A","E2B"]
#define ENCODER_INT_IRQN                                        (GPIOA_INT_IRQn)
#define ENCODER_INT_IIDX                        (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENCODER_E1A_IIDX                                    (DL_GPIO_IIDX_DIO28)
#define ENCODER_E1A_PIN                                         (DL_GPIO_PIN_28)
#define ENCODER_E1A_IOMUX                                         (IOMUX_PINCM3)
/* Defines for E1B: GPIOA.29 with pinCMx 4 on package pin 36 */
#define ENCODER_E1B_IIDX                                    (DL_GPIO_IIDX_DIO29)
#define ENCODER_E1B_PIN                                         (DL_GPIO_PIN_29)
#define ENCODER_E1B_IOMUX                                         (IOMUX_PINCM4)
/* Defines for E2A: GPIOA.30 with pinCMx 5 on package pin 37 */
#define ENCODER_E2A_IIDX                                    (DL_GPIO_IIDX_DIO30)
#define ENCODER_E2A_PIN                                         (DL_GPIO_PIN_30)
#define ENCODER_E2A_IOMUX                                         (IOMUX_PINCM5)
/* Defines for E2B: GPIOA.31 with pinCMx 6 on package pin 39 */
#define ENCODER_E2B_IIDX                                    (DL_GPIO_IIDX_DIO31)
#define ENCODER_E2B_PIN                                         (DL_GPIO_PIN_31)
#define ENCODER_E2B_IOMUX                                         (IOMUX_PINCM6)
/* Port definition for Pin Group A4950 */
#define A4950_PORT                                                       (GPIOA)

/* Defines for AIN1: GPIOA.15 with pinCMx 37 on package pin 8 */
#define A4950_AIN1_PIN                                          (DL_GPIO_PIN_15)
#define A4950_AIN1_IOMUX                                         (IOMUX_PINCM37)
/* Defines for BIN1: GPIOA.16 with pinCMx 38 on package pin 9 */
#define A4950_BIN1_PIN                                          (DL_GPIO_PIN_16)
#define A4950_BIN1_IOMUX                                         (IOMUX_PINCM38)
/* Port definition for Pin Group GRAYSCALE */
#define GRAYSCALE_PORT                                                   (GPIOB)

/* Defines for GRAYSCALE_0: GPIOB.0 with pinCMx 12 on package pin 47 */
#define GRAYSCALE_GRAYSCALE_0_PIN                                (DL_GPIO_PIN_0)
#define GRAYSCALE_GRAYSCALE_0_IOMUX                              (IOMUX_PINCM12)
/* Defines for GRAYSCALE_1: GPIOB.1 with pinCMx 13 on package pin 48 */
#define GRAYSCALE_GRAYSCALE_1_PIN                                (DL_GPIO_PIN_1)
#define GRAYSCALE_GRAYSCALE_1_IOMUX                              (IOMUX_PINCM13)
/* Defines for GRAYSCALE_2: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GRAYSCALE_GRAYSCALE_2_PIN                                (DL_GPIO_PIN_2)
#define GRAYSCALE_GRAYSCALE_2_IOMUX                              (IOMUX_PINCM15)
/* Defines for GRAYSCALE_3: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GRAYSCALE_GRAYSCALE_3_PIN                                (DL_GPIO_PIN_3)
#define GRAYSCALE_GRAYSCALE_3_IOMUX                              (IOMUX_PINCM16)
/* Defines for GRAYSCALE_4: GPIOB.4 with pinCMx 17 on package pin 52 */
#define GRAYSCALE_GRAYSCALE_4_PIN                                (DL_GPIO_PIN_4)
#define GRAYSCALE_GRAYSCALE_4_IOMUX                              (IOMUX_PINCM17)
/* Defines for GRAYSCALE_5: GPIOB.5 with pinCMx 18 on package pin 53 */
#define GRAYSCALE_GRAYSCALE_5_PIN                                (DL_GPIO_PIN_5)
#define GRAYSCALE_GRAYSCALE_5_IOMUX                              (IOMUX_PINCM18)
/* Defines for GRAYSCALE_6: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GRAYSCALE_GRAYSCALE_6_PIN                                (DL_GPIO_PIN_6)
#define GRAYSCALE_GRAYSCALE_6_IOMUX                              (IOMUX_PINCM23)
/* Defines for GRAYSCALE_7: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GRAYSCALE_GRAYSCALE_7_PIN                                (DL_GPIO_PIN_7)
#define GRAYSCALE_GRAYSCALE_7_IOMUX                              (IOMUX_PINCM24)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_I2C_6050_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_UART_2_init(void);
void SYSCFG_DL_SPI_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
