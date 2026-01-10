#include "mpu6050.h"
#include <math.h>
//#include "inv_mpu_dmp_motion_driver.h"
//#include "inv_mpu.h"
#include "usart.h"


#define DEFAULT_MPU_HZ (100)
#define q30 			1073741824.0f

static signed char gyro_orientation[9] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
};

// float q0 = 1.0f , q1 = 0.0f , q2 = 0.0f , q3 = 0.0f;
unsigned long sensor_timestamp;
short gyro[3],accel[3] , sensors;
unsigned char more;
long quat[4];

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK精英STM32开发板V3
//MPU6050 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/17
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
 
//初始化MPU6050
//返回值:0,成功
//    其他,错误代码
uint8_t MPU_Init(void)
{ 
	uint8_t res;
//  GPIO_InitTypeDef  GPIO_InitStructure;
//	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//使能AFIO时钟 
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//先使能外设IO PORTA时钟 
//	
//  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;	 // 端口配置
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
//  GPIO_Init(GPIOA, &GPIO_InitStructure);					 //根据设定参数初始化GPIOA

//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//禁止JTAG,从而PA15可以做普通IO使用,否则PA15不能做普通IO!!!
//	
//	MPU_AD0_CTRL=0;			//控制MPU6050的AD0脚为低电平,从机地址为:0X68
	
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X80);	//复位MPU6050
    HAL_Delay(100);
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X00);	//唤醒MPU6050 
	MPU_Set_Gyro_Fsr(1);					//陀螺仪传感器,±2000dps
	MPU_Set_Accel_Fsr(0);					//加速度传感器,±2g
	MPU_Set_Rate(50);						//设置采样率50Hz
	MPU_Write_Byte(MPU_INT_EN_REG,0X00);	//关闭所有中断
	MPU_Write_Byte(MPU_USER_CTRL_REG,0X00);	//I2C主模式关闭
	MPU_Write_Byte(MPU_FIFO_EN_REG,0X00);	//关闭FIFO
	MPU_Write_Byte(MPU_INTBP_CFG_REG,0X80);	//INT引脚低电平有效
	MPU_Write_Byte(MPU_INTBP_CFG_REG,0X02); //旁路模式
	AK8963_Init();
	res = MPU_Read_Byte(MPU_DEVICE_ID_REG);
	if(res==MPU_ADDR || res == 0x70 || res == 0x71)//器件ID正确
	{
		MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X01);	//设置CLKSEL,PLL X轴为参考
		MPU_Write_Byte(MPU_PWR_MGMT2_REG,0X00);	//加速度与陀螺仪都工作
		MPU_Set_Rate(50);						//设置采样率为50Hz
 	}else return 1;
	HAL_Delay(10);
	HAL_I2C_Mem_Read(&hi2c1,0x0c<<1,0x00,1,&res,1,HAL_MAX_DELAY);
	if(res != 0x48)
	{
		return 2;
	}
	return 0;
}
//设置MPU6050陀螺仪传感器满量程范围
//fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
//返回值:0,设置成功
//    其他,设置失败 

uint8_t AK8963_Init(void)
{
	uint8_t res = 0x16;
	HAL_Delay(10);
	HAL_I2C_Mem_Write(&hi2c1, AK8963_ADDR<<1, MAG_CNTL1, 1,&res, 1, 1000);
	return 0;    
}

uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU_GYRO_CFG_REG,fsr<<3);//设置陀螺仪满量程范围  
}
//设置MPU6050加速度传感器满量程范围
//fsr:0,±2g;1,±4g;2,±8g;3,±16g
//返回值:0,设置成功
//    其他,设置失败 
uint8_t MPU_Set_Accel_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU_ACCEL_CFG_REG,fsr<<3);//设置加速度传感器满量程范围  
}
//设置MPU6050的数字低通滤波器
//lpf:数字低通滤波频率(Hz)
//返回值:0,设置成功
//    其他,设置失败 
uint8_t MPU_Set_LPF(uint16_t lpf)
{
	uint8_t data=0;
	if(lpf>=188)data=1;
	else if(lpf>=98)data=2;
	else if(lpf>=42)data=3;
	else if(lpf>=20)data=4;
	else if(lpf>=10)data=5;
	else data=6; 
	return MPU_Write_Byte(MPU_CFG_REG,data);//设置数字低通滤波器  
}
//设置MPU6050的采样率(假定Fs=1KHz)
//rate:4~1000(Hz)
//返回值:0,设置成功
//    其他,设置失败 
uint8_t MPU_Set_Rate(uint16_t rate)
{
	uint8_t data;
	if(rate>1000)rate=1000;
	if(rate<4)rate=4;
	data=1000/rate-1;
	data=MPU_Write_Byte(MPU_SAMPLE_RATE_REG,data);	//设置数字低通滤波器
 	return MPU_Set_LPF(rate/2);	//自动设置LPF为采样率的一半
}

//得到温度值
//返回值:温度值(扩大了100倍)
short MPU_Get_Temperature(void)
{
    uint8_t buf[2]; 
    short raw;
	float temp;
	MPU_Read_Len(MPU_ADDR,MPU_TEMP_OUTH_REG,2,buf); 
    raw=((uint16_t)buf[0]<<8)|buf[1];  
    temp=36.53+((double)raw)/340;  
    return temp*100;;
}
//得到陀螺仪值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
uint8_t MPU_Get_Gyroscope(short *gx,short *gy,short *gz)
{
    uint8_t buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
	if(res==0)
	{
		*gx=((uint16_t)buf[0]<<8)|buf[1];  
		*gy=((uint16_t)buf[2]<<8)|buf[3];  
		*gz=((uint16_t)buf[4]<<8)|buf[5];
	} 	
    return res;;
}
//得到加速度值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
uint8_t MPU_Get_Accelerometer(short *ax,short *ay,short *az)
{
    uint8_t buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
	if(res==0)
	{
		*ax=((uint16_t)buf[0]<<8)|buf[1];  
		*ay=((uint16_t)buf[2]<<8)|buf[3];  
		*az=((uint16_t)buf[4]<<8)|buf[5];
	} 	
    return res;;
}

uint8_t MPU_Get_Magnetometer(short *mx, short *my, short *mz)
{
	uint8_t buf[7], res;
	res = MPU_Read_Len(AK8963_ADDR, MAG_XOUT_L, 7, buf);
	if (res == 0) {
		*mx = ((uint16_t)buf[1] << 8) | buf[0];
		*my = ((uint16_t)buf[3] << 8) | buf[2];
		*mz = ((uint16_t)buf[5] << 8) | buf[4];
	}
	return res;
}

//IIC连续写
//addr:器件地址 
//reg:寄存器地址
//len:写入长度
//buf:数据区
//返回值:0,正常
//    其他,错误代码
uint8_t MPU_Write_Len(uint8_t addr,uint8_t reg,uint8_t len,uint8_t *buf)
{
	HAL_I2C_Mem_Write(&hi2c1, addr<<1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
	if (HAL_I2C_GetError(&hi2c1) != HAL_OK) {
		return 1; // Error occurred
	}
	return 0; // Success
} 
//IIC连续读
//addr:器件地址
//reg:要读取的寄存器地址
//len:要读取的长度
//buf:读取到的数据存储区
//返回值:0,正常
//    其他,错误代码
uint8_t MPU_Read_Len(uint8_t addr,uint8_t reg,uint8_t len,uint8_t *buf)
{ 
	HAL_I2C_Mem_Read(&hi2c1, addr<<1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
	if (HAL_I2C_GetError(&hi2c1) != HAL_OK) {
		return 1; // Error occurred
	}
	return 0; // Success
}
//IIC写一个字节 
//reg:寄存器地址
//data:数据
//返回值:0,正常
//    其他,错误代码
uint8_t MPU_Write_Byte(uint8_t reg,uint8_t data) 				 
{ 
	
	return HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR<<1, reg, 1, &data, 1, 1000);
}
//IIC读一个字节 
//reg:寄存器地址 
//返回值:读到的数据
uint8_t MPU_Read_Byte(uint8_t reg)
{
	uint8_t res;
	HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR<<1, reg, 1, &res, 1, 1000);
    return res;
}

// uint8_t MPU6050_DMPInit(void)
// {
// 	uint8_t res = 0;
// 	char buf[40];
// 	mpu_set_sensors(INV_XYZ_ACCEL|INV_XYZ_GYRO);
// 	res = mpu_init();
// 	if(!res)
// 	{
// 		res = mpu_set_sensors(INV_XYZ_ACCEL|INV_XYZ_GYRO);
// 		if(!res)
// 		{
// 			sprintf(buf,"set sensors error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 			return 1;
// 		}
// 		res = mpu_configure_fifo(INV_XYZ_GYRO|INV_XYZ_ACCEL);
// 		if(!res)
// 		{
// 			sprintf(buf,"configure_fifo error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 			return 2;
// 		}
// 		res = mpu_set_sample_rate(DEFAULT_MPU_HZ);
// 		if(!res)
// 		{
// 			sprintf(buf,"set_sample_rate error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 			return 3;
// 		}
// 		res = dmp_load_motion_driver_firmware();
// 		if (!res)
// 		{
// 			sprintf(buf,"firmware error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 			return 4;
// 		}
// 		res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
// 		if(!res)
// 		{
// 			sprintf(buf,"set_orientation error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);

// 			return 5;
// 		}
// 		res = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);
// 		if(!res)
// 		{
// 			sprintf(buf,"enable_feature error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 			return 6;
// 		}
// 		res = dmp_set_fifo_rate(DEFAULT_MPU_HZ);
// 		if(!res)
// 		{
// 			sprintf(buf,"set_fifo_rate error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);

// 			return 7;
// 		}
// 		res = mpu_set_dmp_state(1);
// 		if(!res)
// 		{
// 			sprintf(buf,"set_dmp_state error");
// 			HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);

// 			return 8;
// 		}
		
// 	}
// 	else
// 	{
// 		sprintf(buf,"dmp init error %d",res);
// 		HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 0xffff);
// 		return 9;
// 	}	
// }


// uint8_t MPU6050_ReadDMP(float* Yaw,float* Roll,float* Pitch)
// {
// 	if(dmp_read_fifo(gyro,accel,quat,&sensor_timestamp,&sensors,&more))
// 	{
// 		return 1;
// 	}
// 	if(sensors & INV_WXYZ_QUAT)
// 	{
// 		q0 = quat[0] / q30;
// 		q1 = quat[1] / q30;
// 		q2 = quat[2] / q30;
// 		q3 = quat[3] / q30;
// 		*Pitch = asin(-2 * q1 * q3 + 2 * q0 * q2 ) * 57.3;
// 		*Roll = atan2(2 * q2 * q3 + 2 * q0 * q1 , -2 * q1 * q1 - 2 * q2 * q2 +1 ) * 57.3;
// 		*Yaw = atan2(2 * q1 * q2 + 2 * q0 * q3 , -2 * q2 * q2 - 2 * q3 * q3 +1) * 57.3;
// 	}
// 	else
// 		return 2;
// 	return 0;
// }
