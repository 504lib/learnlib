#include "./MPU_6050/mpu6050_i2c_ti.h"
#include "ti_msp_dl_config.h"
#include "./DL_GetTick_Folder/DL_Tick.h"

#define I2C_TIMEOUT_MS  100

void TI_Delay_ms(uint32_t ms)
{
    uint32_t hz = CPUCLK_FREQ / (1000 / ms);
    delay_cycles(hz);
}

/* 参考 TI SDK I2C_ReadReg 实现：
 *   1. TX 发寄存器地址（带 STOP）
 *   2. IDLE 等待 + 清 TX FIFO
 *   3. RX 读数据
 */

/* 发送：等 IDLE → 填 FIFO → 发起传输 → 等 BUSY → 等 IDLE → 清 FIFO */
static uint8_t i2c_tx(uint8_t addr, const uint8_t *d, uint8_t len)
{
    /* 等待控制器空闲 */
    while (!(DL_I2C_getControllerStatus(I2C_6050_INST)
           & DL_I2C_CONTROLLER_STATUS_IDLE));

    DL_I2C_fillControllerTXFIFO(I2C_6050_INST, d, len);

    DL_I2C_startControllerTransfer(I2C_6050_INST, addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, len);

    while (DL_I2C_getControllerStatus(I2C_6050_INST)
           & DL_I2C_CONTROLLER_STATUS_BUSY);

    while (!(DL_I2C_getControllerStatus(I2C_6050_INST)
           & DL_I2C_CONTROLLER_STATUS_IDLE));

    if (DL_I2C_getControllerStatus(I2C_6050_INST)
        & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_flushControllerTXFIFO(I2C_6050_INST);
        return 1;
    }

    DL_I2C_flushControllerTXFIFO(I2C_6050_INST);
    return 0;
}

/* 接收：等 IDLE → 发起传输 → 读 FIFO（TI 示例在 RX 后不等待 BUSY） */
static uint8_t i2c_rx(uint8_t addr, uint8_t *d, uint8_t len)
{
    while (!(DL_I2C_getControllerStatus(I2C_6050_INST)
           & DL_I2C_CONTROLLER_STATUS_IDLE));

    DL_I2C_startControllerTransfer(I2C_6050_INST, addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, len);

    for (uint8_t i = 0; i < len; i++) {
        while (DL_I2C_isControllerRXFIFOEmpty(I2C_6050_INST));
        d[i] = DL_I2C_receiveControllerData(I2C_6050_INST);
    }

    while (DL_I2C_getControllerStatus(I2C_6050_INST)
           & DL_I2C_CONTROLLER_STATUS_BUSY);
    return 0;
}

/* ===== 公开 API ===== */

uint8_t TI_I2C_WriteByte(uint8_t dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_tx(dev, buf, 2);
}

uint8_t TI_I2C_ReadByte(uint8_t dev, uint8_t reg, uint8_t *data)
{
    /* 第1步：写寄存器地址（带 STOP） */
    uint8_t ret = i2c_tx(dev, &reg, 1);
    if (ret) return ret;

    /* 第2步：读1字节 */
    return i2c_rx(dev, data, 1);
}

uint8_t TI_I2C_WriteLen(uint8_t dev, uint8_t reg, uint8_t len, uint8_t *buf)
{
    uint8_t tmp[32];
    tmp[0] = reg;
    for (uint8_t i = 0; i < len; i++) tmp[i + 1] = buf[i];
    return i2c_tx(dev, tmp, len + 1);
}

uint8_t TI_I2C_ReadLen(uint8_t dev, uint8_t reg, uint8_t len, uint8_t *buf)
{
    /* 第1步：写寄存器地址（带 STOP） */
    uint8_t ret = i2c_tx(dev, &reg, 1);
    if (ret) return ret;

    /* 第2步：读 len 字节 */
    return i2c_rx(dev, buf, len);
}
