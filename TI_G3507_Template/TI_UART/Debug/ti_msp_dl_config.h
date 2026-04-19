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
#define PWM_0_INST                                                         TIMG6
#define PWM_0_INST_IRQHandler                                   TIMG6_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMG6_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                         DL_GPIO_PIN_21
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM46)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM46_PF_TIMG6_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                         DL_GPIO_PIN_22
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM47)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM47_PF_TIMG6_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX

/* Defines for PWM_1 */
#define PWM_1_INST                                                         TIMG0
#define PWM_1_INST_IRQHandler                                   TIMG0_IRQHandler
#define PWM_1_INST_INT_IRQN                                     (TIMG0_INT_IRQn)
#define PWM_1_INST_CLK_FREQ                                              4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_1_C0_PORT                                                 GPIOA
#define GPIO_PWM_1_C0_PIN                                         DL_GPIO_PIN_12
#define GPIO_PWM_1_C0_IOMUX                                      (IOMUX_PINCM34)
#define GPIO_PWM_1_C0_IOMUX_FUNC                     IOMUX_PINCM34_PF_TIMG0_CCP0
#define GPIO_PWM_1_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_1_C1_PORT                                                 GPIOA
#define GPIO_PWM_1_C1_PIN                                         DL_GPIO_PIN_13
#define GPIO_PWM_1_C1_IOMUX                                      (IOMUX_PINCM35)
#define GPIO_PWM_1_C1_IOMUX_FUNC                     IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_PWM_1_C1_IDX                                    DL_TIMER_CC_1_INDEX



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




/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_0
#define GPIO_I2C_0_IOMUX_SDA                                      (IOMUX_PINCM1)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                       IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_1
#define GPIO_I2C_0_IOMUX_SCL                                      (IOMUX_PINCM2)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                       IOMUX_PINCM2_PF_I2C0_SCL


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




/* Defines for SPI */
#define SPI_INST                                                           SPI1
#define SPI_INST_IRQHandler                                     SPI1_IRQHandler
#define SPI_INST_INT_IRQN                                         SPI1_INT_IRQn
#define GPIO_SPI_PICO_PORT                                                GPIOB
#define GPIO_SPI_PICO_PIN                                         DL_GPIO_PIN_8
#define GPIO_SPI_IOMUX_PICO                                     (IOMUX_PINCM25)
#define GPIO_SPI_IOMUX_PICO_FUNC                     IOMUX_PINCM25_PF_SPI1_PICO
#define GPIO_SPI_POCI_PORT                                                GPIOB
#define GPIO_SPI_POCI_PIN                                         DL_GPIO_PIN_7
#define GPIO_SPI_IOMUX_POCI                                     (IOMUX_PINCM24)
#define GPIO_SPI_IOMUX_POCI_FUNC                     IOMUX_PINCM24_PF_SPI1_POCI
/* GPIO configuration for SPI */
#define GPIO_SPI_SCLK_PORT                                                GPIOB
#define GPIO_SPI_SCLK_PIN                                         DL_GPIO_PIN_9
#define GPIO_SPI_IOMUX_SCLK                                     (IOMUX_PINCM26)
#define GPIO_SPI_IOMUX_SCLK_FUNC                     IOMUX_PINCM26_PF_SPI1_SCLK



/* Port definition for Pin Group LED1 */
#define LED1_PORT                                                        (GPIOB)

/* Defines for PIN_22: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED1_PIN_22_PIN                                         (DL_GPIO_PIN_22)
#define LED1_PIN_22_IOMUX                                        (IOMUX_PINCM50)
/* Port definition for Pin Group KEY_GROIP */
#define KEY_GROIP_PORT                                                   (GPIOB)

/* Defines for BOARD_KEY: GPIOB.21 with pinCMx 49 on package pin 20 */
#define KEY_GROIP_BOARD_KEY_PIN                                 (DL_GPIO_PIN_21)
#define KEY_GROIP_BOARD_KEY_IOMUX                                (IOMUX_PINCM49)
/* Port definition for Pin Group SPI_OLED_PIN */
#define SPI_OLED_PIN_PORT                                                (GPIOB)

/* Defines for RES_PIN: GPIOB.10 with pinCMx 27 on package pin 62 */
#define SPI_OLED_PIN_RES_PIN_PIN                                (DL_GPIO_PIN_10)
#define SPI_OLED_PIN_RES_PIN_IOMUX                               (IOMUX_PINCM27)
/* Defines for DC_PIN: GPIOB.11 with pinCMx 28 on package pin 63 */
#define SPI_OLED_PIN_DC_PIN_PIN                                 (DL_GPIO_PIN_11)
#define SPI_OLED_PIN_DC_PIN_IOMUX                                (IOMUX_PINCM28)
/* Defines for CS_PIN: GPIOB.14 with pinCMx 31 on package pin 2 */
#define SPI_OLED_PIN_CS_PIN_PIN                                 (DL_GPIO_PIN_14)
#define SPI_OLED_PIN_CS_PIN_IOMUX                                (IOMUX_PINCM31)
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
/* Defines for AIN1: GPIOB.13 with pinCMx 30 on package pin 1 */
#define TB6612_AIN1_PORT                                                 (GPIOB)
#define TB6612_AIN1_PIN                                         (DL_GPIO_PIN_13)
#define TB6612_AIN1_IOMUX                                        (IOMUX_PINCM30)
/* Defines for AIN2: GPIOB.15 with pinCMx 32 on package pin 3 */
#define TB6612_AIN2_PORT                                                 (GPIOB)
#define TB6612_AIN2_PIN                                         (DL_GPIO_PIN_15)
#define TB6612_AIN2_IOMUX                                        (IOMUX_PINCM32)
/* Defines for BIN1: GPIOB.16 with pinCMx 33 on package pin 4 */
#define TB6612_BIN1_PORT                                                 (GPIOB)
#define TB6612_BIN1_PIN                                         (DL_GPIO_PIN_16)
#define TB6612_BIN1_IOMUX                                        (IOMUX_PINCM33)
/* Defines for BIN2: GPIOA.14 with pinCMx 36 on package pin 7 */
#define TB6612_BIN2_PORT                                                 (GPIOA)
#define TB6612_BIN2_PIN                                         (DL_GPIO_PIN_14)
#define TB6612_BIN2_IOMUX                                        (IOMUX_PINCM36)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_PWM_1_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_SPI_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
