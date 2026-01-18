#include "mpu6050_user.h"
#include <math.h>
#include "mpu6050.h"

//extern tPid pid_yaw;
//extern float gz_offset;
float gz_filtered = 0;
float alpha = 0.85f; // 低通滤波系数（0.7~0.9 可调）
float yaw_angle = 0.0f;


#define RAD_TO_DEG 57.2957795131f
#define CALIB_SAMPLES 500
#define DEG_TO_RAD 0.0174532925f

int16_t mag_x_max = -32768, mag_y_max = -32768, mag_z_max = -32768;
int16_t mag_x_min =  32767, mag_y_min =  32767, mag_z_min =  32767;


static Gyro_offset gyro_offset = {0};
static Accelero_offset accelero_offset = {0};
static Magneto_offset magneto_offset = {0};
static Euler_Angles euler_angles = {0};
static Gyro_Data gyro_data = {0};
static Accelero_Data accelero_data = {0};
static Magneto_Data magneto_data = {0};
static MPU6050_Offsets mpu6050_offsets_structure = {0};
static Scale scale_data = {0};
static MPU6050_Data mpu6050_data_structure = {0};

MPU6050_Data* MPU6050_Init_Data(void)
{
    mpu6050_data_structure.gyro = &gyro_data;
    mpu6050_data_structure.accelero = &accelero_data;
    mpu6050_data_structure.magneto = &magneto_data;
    mpu6050_offsets_structure.euler_angles = &euler_angles;
    mpu6050_data_structure.offsets = &mpu6050_offsets_structure;

    mpu6050_data_structure.scale = &scale_data;

    mpu6050_data_structure.offsets->accelero_offset = &accelero_offset;
    mpu6050_data_structure.offsets->gyro_offset = &gyro_offset;
    mpu6050_data_structure.offsets->magneto_offset = &magneto_offset;

    return &mpu6050_data_structure;
}

static void Calibrate_Magnetometer(MPU6050_Data* mpu6050_data) {
    int16_t mx, my, mz;
    char buf[20];
    for (int i = 0; i < CALIB_SAMPLES; i++) {
        sprintf(buf,"calibrating %d times...",i);
        OLED_ShowString(0,4,(uint8_t*)buf,16,1);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 0xffff);
        MPU_Get_Magnetometer(&mx, &my, &mz); // 用户需要实现
        if (mx > mag_x_max) mag_x_max = mx;
        if (mx < mag_x_min) mag_x_min = mx;

        if (my > mag_y_max) mag_y_max = my;
        if (my < mag_y_min) mag_y_min = my;

        if (mz > mag_z_max) mag_z_max = mz;
        if (mz < mag_z_min) mag_z_min = mz;

        HAL_Delay(10); // 每秒采样50次，持续10秒
    }

    mpu6050_data->offsets->magneto_offset->mx_offset = (mag_x_max + mag_x_min) / 2.0f;
    mpu6050_data->offsets->magneto_offset->my_offset = (mag_y_max + mag_y_min) / 2.0f;
    mpu6050_data->offsets->magneto_offset->mz_offset = (mag_z_max + mag_z_min) / 2.0f;

    float scale_x = (mag_x_max - mag_x_min) / 2.0f;
    float scale_y = (mag_y_max - mag_y_min) / 2.0f;
    float scale_z = (mag_z_max - mag_z_min) / 2.0f;

    float max_scale = scale_x;
    if (scale_y > max_scale) max_scale = scale_y;
    if (scale_z > max_scale) max_scale = scale_z;
    sprintf(buf,"max_scale:%0.2f\n",max_scale);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 0xffff);


    mpu6050_data->scale->scale_x = scale_x / max_scale;
    mpu6050_data->scale->scale_y = scale_y / max_scale;
    mpu6050_data->scale->scale_z = scale_z / max_scale;
}

static void Magnetometer_Filter(MPU6050_Data* mpu6050_data) 
{
    #define FILTER_SIZE 20
    static uint8_t index = 0;
    short mx_raw, my_raw, mz_raw;
    static float mx_buf[FILTER_SIZE] = {0};
    static float my_buf[FILTER_SIZE] = {0};
    static float mz_buf[FILTER_SIZE] = {0};
    MPU_Get_Magnetometer(&mx_raw, &my_raw, &mz_raw);
    mx_buf[index] = mx_raw;
    my_buf[index] = my_raw;
    mz_buf[index] = mz_raw;
    index = (index + 1) % FILTER_SIZE;
    float mx_sum = 0, my_sum = 0, mz_sum = 0;
    for (uint8_t i = 0; i < FILTER_SIZE; i++) {
        mx_sum += mx_buf[i];
        my_sum += my_buf[i];
        mz_sum += mz_buf[i];
    }
    mpu6050_data->magneto->mx = (mx_sum / FILTER_SIZE - mpu6050_data->offsets->magneto_offset->mx_offset) * mpu6050_data->scale->scale_x;
    mpu6050_data->magneto->my = (my_sum / FILTER_SIZE - mpu6050_data->offsets->magneto_offset->my_offset) * mpu6050_data->scale->scale_y;
    mpu6050_data->magneto->mz = (mz_sum / FILTER_SIZE - mpu6050_data->offsets->magneto_offset->mz_offset) * mpu6050_data->scale->scale_z;
}


void Get_yaw_ComGyroMagnetometer(MPU6050_Data* mpu6050_data, float* yaw , float* pitch , float* roll , float dt) {
    static float last_yaw = 0;
    static float yaw_gyro = 0;
    static uint8_t first_run = 1;
    // static float yaw_offset = 0;
    float yaw_fused = 0;
    float yaw_mag = 0;
    Magnetometer_Filter(mpu6050_data);
    float mx = mpu6050_data->magneto->mx;
    float my = mpu6050_data->magneto->my;

    // if (mx == 0 && my == 0) {
    //     return; // 避免除以零
    // }
    yaw_mag = atan2(my, mx) * RAD_TO_DEG; 
    // if(first_run)
    // {
    //     yaw_offset = yaw_mag;
    //     first_run = 0;
    // }
    // if(fabs(mpu6050_data->gyro->gz) > 5.0f)
    // {
        yaw_gyro += mpu6050_data->gyro->gz * dt * RAD_TO_DEG;
        yaw_fused = yaw_gyro * alpha + yaw_mag * (1 - alpha);
    // }
    // else
    // {
        // yaw_fused = yaw_mag * alpha + last_yaw * (1 - alpha);
    // }
    // float diff = yaw_mag - yaw_gyro;
    // if (diff > 180) diff -= 360;
    // if (diff < -180) diff += 360;

    // yaw_fused = alpha * yaw_gyro + (1 - alpha) * yaw_mag;
    // if (yaw_fused < 0) yaw_fused += 360; // 保证输出为0~360度
    // else if (yaw_fused >= 360) yaw_fused -= 360; // 保证输出为0~360度       
    // 平滑处理
    // *yaw = alpha * last_yaw + (1 - alpha) * yaw_fused;
    *yaw = yaw_mag;
    if (*yaw < 0) *yaw += 360; // 保证输出为0~360度
    else if (*yaw >= 360) *yaw -= 360; // 保证输出为0~360度
    // 输出当前yaw角度
    if (fabs(*yaw - last_yaw) > 1.0f) { // 如果变化超过1度才更新
        last_yaw = *yaw;
    }
}


// 延时函数（OLED需要）
static void Delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

// 零飘校准（上电静止执行）
static void MPU6050_Calibrate(MPU6050_Data* mpu6050_data)
{
    float yaw, pitch, roll;
    float yaw_sum = 0;
    float pitch_sum = 0;
    float roll_sum = 0;
    char buf[40];
    OLED_Clear();
    short gx, gy, gz;
    short ax, ay, az;
    int i;
    int32_t gx_sum = 0;
    int32_t gy_sum = 0;
    int32_t gz_sum = 0;
    int32_t ax_sum = 0;
    int32_t ay_sum = 0;
    int32_t az_sum = 0;
    sprintf(buf,"calibrate_magneto ing ...");
    OLED_ShowString(0,0,(uint8_t*)buf,16,1);
    Calibrate_Magnetometer(mpu6050_data);
    OLED_Clear();
    sprintf(buf,"calibrate_Gyro and Accelerometer ing ...");
    OLED_ShowString(0,0,(uint8_t*)buf,16,1);
    for (i = 0; i < 200; i++) {
        MPU_Get_Gyroscope(&gx, &gy, &gz);
        gx_sum += gx;
        gy_sum += gy;
        gz_sum += gz;
        MPU_Get_Accelerometer(&ax, &ay, &az);
        ax_sum += ax;
        ay_sum += ay;
        az_sum += az;
        Delay_ms(5);
    }
    OLED_Clear();
    mpu6050_data->offsets->gyro_offset->gx_offset = gx_sum / 200.0f;
    mpu6050_data->offsets->gyro_offset->gy_offset = gy_sum / 200.0f;
    mpu6050_data->offsets->gyro_offset->gz_offset = gz_sum / 200.0f;
    mpu6050_data->offsets->accelero_offset->ax_offset = ax_sum / 200.0f;
    mpu6050_data->offsets->accelero_offset->ay_offset = ay_sum / 200.0f;
    mpu6050_data->offsets->accelero_offset->az_offset = az_sum / 200.0f;

    // for (uint8_t i = 0; i < 20; i++)
    // {
    //     IMU_FusionUpdate(mpu6050_data, 0.01f, &roll, &pitch, &yaw);
    //     yaw_sum += yaw;
    //     pitch_sum += pitch;
    //     roll_sum += roll;
    // }
    // mpu6050_data->offsets->euler_angles->yaw = yaw_sum / 20.0f;
    // mpu6050_data->offsets->euler_angles->pitch = pitch_sum / 20.0f;
    // mpu6050_data->offsets->euler_angles->roll = roll_sum / 20.0f;
    
}


static void MPU6050_Debug(void)
{
    OLED_Clear();

    uint8_t id = MPU_Read_Byte(0x75);
    char buf[20];
    sprintf(buf, "ID=0x%02X", id);
    OLED_ShowString(0, 0, (uint8_t*)"MPU6050 FAIL", 16,1);
    OLED_ShowString(0, 2, (uint8_t*)buf, 16,1);
    OLED_ShowString(0, 4, (uint8_t*)"Check SCL/SDA", 16,1);
    OLED_ShowString(0, 6, (uint8_t*)"ADDR=0x68?", 16,1);
}

static void AK8963_Debug(void)
{
    OLED_Clear();
    uint8_t ak8963_id = 0;
    HAL_I2C_Mem_Read(&hi2c1,0x0c<<1,0x00,1,&ak8963_id,1,HAL_MAX_DELAY);
    char buf[20];
    sprintf(buf, "ID=0x%02X", ak8963_id);
    OLED_ShowString(0, 0, (uint8_t*)"AK8963 FAIL", 16,1);
    OLED_ShowString(0, 2, (uint8_t*)buf, 16,1);
    OLED_ShowString(0, 4, (uint8_t*)"Check SCL/SDA", 16,1);
    OLED_ShowString(0, 6, (uint8_t*)"ADDR=0x0C?", 16,1);
}

void MPU6050_Test_Display(MPU6050_Data* mpu6050_data)
{
    uint8_t res = MPU_Init();
    if (res == 0) {
        OLED_ShowString(0, 0, (uint8_t*)"MPU6050 OK", 16,1);
    } else if(res == 1){
        MPU6050_Debug();
    } else if(res == 2){
        AK8963_Debug();
    }

	HAL_Delay(3000);

//    MPU6050_Calibrate(mpu6050_data);
    OLED_ShowString(0, 0, (uint8_t*)"Calib Done", 16,1);
    Delay_ms(1000);
    OLED_Clear();
}

void get_gyro_filtered(MPU6050_Data* mpu6050_data) {
    static float filtered_gx = 0;
    static float filtered_gy = 0;
    static float filtered_gz = 0;
    short gx_raw, gy_raw, gz_raw;

    MPU_Get_Gyroscope(&gx_raw, &gy_raw, &gz_raw);
    float gx_corr = gx_raw - mpu6050_data->offsets->gyro_offset->gx_offset;
    float gy_corr = gy_raw - mpu6050_data->offsets->gyro_offset->gy_offset;
    float gz_corr = gz_raw - mpu6050_data->offsets->gyro_offset->gz_offset;

    float alpha = 0.4f;

    filtered_gx = alpha * filtered_gx + (1.0f - alpha) * gx_corr;
    filtered_gy = alpha * filtered_gy + (1.0f - alpha) * gy_corr;
    filtered_gz = alpha * filtered_gz + (1.0f - alpha) * gz_corr;

    mpu6050_data->gyro->gz = filtered_gz * 2000.0f / 32768.0f * DEG_TO_RAD;
    mpu6050_data->gyro->gx = filtered_gx * 2000.0f / 32768.0f * DEG_TO_RAD;
    mpu6050_data->gyro->gy = filtered_gy * 2000.0f / 32768.0f * DEG_TO_RAD;
}

void get_accelero_filtered(MPU6050_Data* mpu6050_data) {
    static float filtered_ax = 0;
    static float filtered_ay = 0;
    static float filtered_az = 0;
    short ax_raw, ay_raw, az_raw;

    MPU_Get_Accelerometer(&ax_raw, &ay_raw, &az_raw);
    float ax_corr = ax_raw - mpu6050_data->offsets->accelero_offset->ax_offset;
    float ay_corr = ay_raw - mpu6050_data->offsets->accelero_offset->ay_offset;
    float az_corr = az_raw - mpu6050_data->offsets->accelero_offset->az_offset;

    float alpha = 0.85f;

    filtered_ax = alpha * filtered_ax + (1.0f - alpha) * ax_corr;
    filtered_ay = alpha * filtered_ay + (1.0f - alpha) * ay_corr;
    filtered_az = alpha * filtered_az + (1.0f - alpha) * az_corr;

    mpu6050_data->accelero->az = filtered_az  / 16384.0f * 9.8f;
    mpu6050_data->accelero->ax = filtered_ax  / 16384.0f * 9.8f;
    mpu6050_data->accelero->ay = filtered_ay  / 16384.0f * 9.8f;
}

void update_MPU6050_Data(MPU6050_Data* mpu6050_data) 
{
    get_gyro_filtered(mpu6050_data);
    get_accelero_filtered(mpu6050_data);
    Magnetometer_Filter(mpu6050_data);
}
