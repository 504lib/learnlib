#include "pid.h"


//定义一个结构体类型变量
tPid pidMotor1Speed;
tPid pidMotor2Speed;
tPid pidMpu6050;
tPid pid_yaw;
tPid pid_grayscale;

//给结构体类型变量赋初值
// pid.c
void PID_init() {
    // 电机1 PID初始化
    pidMotor1Speed.actual_val = 0.0;
    pidMotor1Speed.target_val = 1.0;
    pidMotor1Speed.err = 0.0;
    pidMotor1Speed.err_last = 0.0;
    pidMotor1Speed.err_sum = 0.0;
    pidMotor1Speed.Kp = 18.5;
    pidMotor1Speed.Ki = 0.8;
    pidMotor1Speed.Kd = 3.4;
    pidMotor1Speed.max_integral = 100.0;  // 积分限幅
    pidMotor1Speed.max_derivative = 20.0;
    pidMotor1Speed.max_output = 100.0;    // 输出限幅
    pidMotor1Speed.min_output =0;
    pidMotor1Speed.last_output = 0.0;
    pidMotor1Speed.max_delta = 20.0;      // 每次最大变化量
    pidMotor1Speed.deadband = 0.5;        // 死区（可选）

    // 电机2 PID初始化（同上）
    pidMotor2Speed.actual_val = 0.0;
    pidMotor2Speed.target_val = 1.0;
    pidMotor2Speed.err = 0.0;
    pidMotor2Speed.err_last = 0.0;
    pidMotor2Speed.err_sum = 0.0;
    pidMotor2Speed.Kp = 18.5;
    pidMotor2Speed.Ki = 0.8;
    pidMotor2Speed.Kd = 3.4;
    pidMotor2Speed.max_integral = 100.0;
    pidMotor2Speed.max_derivative = 20.0;
    pidMotor2Speed.max_output = 100.0;
    pidMotor2Speed.min_output = 0;
    pidMotor2Speed.last_output = 0.0;
    pidMotor2Speed.max_delta = 20.0;      // 每次最大变化量
    pidMotor2Speed.deadband = 0.5;        // 死区（可选）

    pidMpu6050.actual_val = 0.0;
    pidMpu6050.target_val = 0.00;
    pidMpu6050.err = 0.0;
    pidMpu6050.err_last = 0.0;
    pidMpu6050.err_sum = 0.0;
    pidMpu6050.Kp = 0.1;
    pidMpu6050.Ki = 0.005;
    pidMpu6050.Kd = 0.08;
    pidMpu6050.max_integral = 100.0;
    pidMpu6050.max_output = 100.0;
    pidMpu6050.min_output = -100.0f;
    pidMpu6050.last_output = 0.0;
    pidMpu6050.max_delta = 0.0;      // 每次最大变化量
    pidMpu6050.deadband = 0.0;        // 死区（可选）

    pid_yaw.actual_val = 0.0;
    pid_yaw.target_val = 0.00;
    pid_yaw.err = 0.0;
    pid_yaw.err_last = 0.0;
    pid_yaw.err_sum = 0.0;
    pid_yaw.Kp = 0.5;
    pid_yaw.Ki = 0.5;
    pid_yaw.Kd = 5;
    pid_yaw.max_integral = 100.0;
    pid_yaw.max_output = 100.0;
    pid_yaw.min_output = -100.0f;
    pid_yaw.last_output = 0.0;
    pid_yaw.max_delta = 0.0;      // 每次最大变化量
    pid_yaw.deadband = 0.0;        // 死区（可选）

    pid_grayscale.actual_val = 0.0;
    pid_grayscale.target_val = 0.00;
    pid_grayscale.err = 0.0;
    pid_grayscale.err_last = 0.0;
    pid_grayscale.err_sum = 0.0;
    pid_grayscale.Kp = 10;
    pid_grayscale.Ki = 0.01;
    pid_grayscale.Kd = 20.0;
    pid_grayscale.max_integral = 100.0;
    pid_grayscale.max_output = 100.0;
    pid_grayscale.min_output = -100.0f;
    pid_grayscale.last_output = 0.0;
    pid_grayscale.max_delta = 0.0;      // 每次最大变化量
    pid_grayscale.deadband = 0.0;        // 死区（可选）
}

//比例p调节控制函数
float P_realize(tPid * pid,float actual_val)
{
	pid->actual_val = actual_val;//传递真实值
	pid->err = pid->target_val - pid->actual_val;//当前误差=目标值-真实值
	//比例控制调节   输出=Kp*当前误差
	pid->actual_val = pid->Kp*pid->err;
	return pid->actual_val;
}
//比例P 积分I 控制函数
float PI_realize(tPid * pid,float actual_val)
{
	pid->actual_val = actual_val;//传递真实值
	pid->err = pid->target_val - pid->actual_val;//当前误差=目标值-真实值
	pid->err_sum += pid->err;//误差累计值 = 当前误差累计和
	//使用PI控制 输出=Kp*当前误差+Ki*误差累计值
	pid->actual_val = pid->Kp*pid->err + pid->Ki*pid->err_sum;
	
	return pid->actual_val;
}
// PID控制函数
// 修改后的 PID_realize 函数
// pid.c
float PID_realize(tPid *pid, float actual_val) {
    pid->actual_val = actual_val;
    pid->err = pid->target_val - pid->actual_val;

    // 方向一致性检查：目标与实际速度方向相反时，清零积分
    if ((pid->target_val > 0 && pid->actual_val < -1.0) || 
        (pid->target_val < 0 && pid->actual_val > 1.0)) {
        pid->err_sum = 0;  // 清零积分项
    }
    // if (fabsf(pid->err) < pid->deadband) {
    //     pid->err = 0;
    // }
    if(actual_val == 0 && pid->err_sum != 0)
    {
        pid->err_sum *= 0.9;
    }
    // 积分项累加（带限幅）
    pid->err_sum += pid->err * pid->Ki;
    if (pid->err_sum > pid->max_integral) {
        pid->err_sum = pid->max_integral;
    } else if (pid->err_sum < -pid->max_integral) {
        pid->err_sum = -pid->max_integral;
    }
    // if ((pid->err_last > 0 && pid->err < 0) || (pid->err_last < 0 && pid->err > 0)) {
    //     pid->err_sum = 0;  // 错误方向发生变化时清零积分，避免过冲
    // }
    // 微分项计算（带限幅）
    float d_err = pid->err - pid->err_last;
    if (d_err > pid->max_derivative) {
        d_err = pid->max_derivative;
    } else if (d_err < -pid->max_derivative) {
        d_err = -pid->max_derivative;
    }
    float derivative = d_err * pid->Kd;

    // 计算PID输出
    float output = pid->Kp * pid->err + pid->err_sum + derivative;

    // float delta = output - pid->last_output;
    // float max_delta = pid->max_delta;  // 新增项
    // if (delta > max_delta) delta = max_delta;
    // if (delta < -max_delta) delta = -max_delta;
    // output = pid->last_output + delta;

    // 输出限幅
    if (output > pid->max_output) 
    {
        output = pid->max_output;
    } 
    else if (output < pid->min_output) 
    {
        output = pid->min_output;
    }

    // 更新误差历史
    pid->err_last = pid->err;
    return output;
}


float PID_Angle(tPid *pid, float actual_yaw, float target_yaw) {
    float err = target_yaw - actual_yaw;
    pid->target_val = target_yaw;  // 更新目标值
    pid->actual_val = actual_yaw;
    // 修正误差为 [-180, 180]
    while (err > 180) err -= 360;
    while (err < -180) err += 360;

    pid->err = err;

    // 积分清零策略（当误差符号变换或太小时清零）
    if (fabsf(err) < 1.0f || (pid->err * pid->err_last < 0)) {
        pid->err_sum = 0;
    } else {
        pid->err_sum += pid->Ki * err;
    }

    // 限制积分
    if (pid->err_sum > pid->max_integral) pid->err_sum = pid->max_integral;
    if (pid->err_sum < -pid->max_integral) pid->err_sum = -pid->max_integral;

    // 计算输出
    float output = pid->Kp * err + pid->err_sum + pid->Kd * (err - pid->err_last);
    pid->err_last = err;

    // 限制输出
    if (output > pid->max_output) output = pid->max_output;
    if (output < -pid->max_output) output = -pid->max_output;

    return output;
}

