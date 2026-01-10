#include "pid.h"


//����һ���ṹ�����ͱ���
tPid pidMotor1Speed;
tPid pidMotor2Speed;
tPid pidMpu6050;
tPid pid_yaw;
tPid pid_grayscale;

//���ṹ�����ͱ�������ֵ
// pid.c
void PID_init() {
    // ���1 PID��ʼ��
    pidMotor1Speed.actual_val = 0.0;
    pidMotor1Speed.target_val = 0.4;
    pidMotor1Speed.err = 0.0;
    pidMotor1Speed.err_last = 0.0;
    pidMotor1Speed.err_sum = 0.0;
    pidMotor1Speed.Kp = 2400.0;
    pidMotor1Speed.Ki = 20.0;
    pidMotor1Speed.Kd = 100.0;
    pidMotor1Speed.max_integral = 1000.0;  // �����޷�
    pidMotor1Speed.max_derivative = 20.0;
    pidMotor1Speed.max_output = 1000.0;    // ����޷�
    pidMotor1Speed.min_output =0;
    pidMotor1Speed.last_output = 0.0;
    pidMotor1Speed.max_delta = 20.0;      // ÿ�����仯��
    pidMotor1Speed.deadband = 0.5;        // ��������ѡ��

    // ���2 PID��ʼ����ͬ�ϣ�
    pidMotor2Speed.actual_val = 0.0;
    pidMotor2Speed.target_val = 0.4;
    pidMotor2Speed.err = 0.0;
    pidMotor2Speed.err_last = 0.0;
    pidMotor2Speed.err_sum = 0.0;
    pidMotor2Speed.Kp = 2400.0;
    pidMotor2Speed.Ki = 20.0;
    pidMotor2Speed.Kd = 100.0;
    pidMotor2Speed.max_integral = 1000.0;
    pidMotor2Speed.max_derivative = 20.0;
    pidMotor2Speed.max_output = 1000.0;
    pidMotor2Speed.min_output = 0;
    pidMotor2Speed.last_output = 0.0;
    pidMotor2Speed.max_delta = 20.0;      // ÿ�����仯��
    pidMotor2Speed.deadband = 0.5;        // ��������ѡ��

    pidMpu6050.actual_val = 0.0;
    pidMpu6050.target_val = 0.00;
    pidMpu6050.err = 0.0;
    pidMpu6050.err_last = 0.0;
    pidMpu6050.err_sum = 0.0;
    pidMpu6050.Kp = 20.0;
    pidMpu6050.Ki = 0.005;
    pidMpu6050.Kd = 0.8;
    pidMpu6050.max_integral = 100.0;
    pidMpu6050.max_output = 100.0;
    pidMpu6050.min_output = -100.0f;
    pidMpu6050.last_output = 0.0;
    pidMpu6050.max_delta = 0.0;      // ÿ�����仯��
    pidMpu6050.deadband = 0.0;        // ��������ѡ��

    pid_yaw.actual_val = 0.0;
    pid_yaw.target_val = 0.00;
    pid_yaw.err = 0.0;
    pid_yaw.err_last = 0.0;
    pid_yaw.err_sum = 0.0;
    pid_yaw.Kp = 2.0;
    pid_yaw.Ki = 0.02;
    pid_yaw.Kd = 1.5;
    pid_yaw.max_integral = 1.0;
    pid_yaw.max_output = 0.6;
    pid_yaw.min_output = -0.6f;
    pid_yaw.last_output = 0.0;
    pid_yaw.max_delta = 0.0;      // ÿ�����仯��
    pid_yaw.deadband = 0.0;        // ��������ѡ��

    pid_grayscale.actual_val = 0.0;
    pid_grayscale.target_val = 0.00;
    pid_grayscale.err = 0.0;
    pid_grayscale.err_last = 0.0;
    pid_grayscale.err_sum = 0.0;
    pid_grayscale.Kp = 1.2f;
    pid_grayscale.Ki = 0.05f;
    pid_grayscale.Kd = 2.0f;
    pid_grayscale.max_integral = 2.0;
    pid_grayscale.max_output = 200.0f;
    pid_grayscale.min_output = -200.0f;
    pid_grayscale.last_output = 0.0;
    pid_grayscale.max_delta = 0.0;      // ÿ�����仯��
    pid_grayscale.deadband = 0.0;        // ��������ѡ��
}

//����p���ڿ��ƺ���
float P_realize(tPid * pid,float actual_val)
{
	pid->actual_val = actual_val;//������ʵֵ
	pid->err = pid->target_val - pid->actual_val;//��ǰ���=Ŀ��ֵ-��ʵֵ
	//�������Ƶ���   ���=Kp*��ǰ���
	pid->actual_val = pid->Kp*pid->err;
	return pid->actual_val;
}
//����P ����I ���ƺ���
float PI_realize(tPid * pid,float actual_val)
{
	pid->actual_val = actual_val;//������ʵֵ
	pid->err = pid->target_val - pid->actual_val;//��ǰ���=Ŀ��ֵ-��ʵֵ
	pid->err_sum += pid->err;//����ۼ�ֵ = ��ǰ����ۼƺ�
	//ʹ��PI���� ���=Kp*��ǰ���+Ki*����ۼ�ֵ
	pid->actual_val = pid->Kp*pid->err + pid->Ki*pid->err_sum;
	
	return pid->actual_val;
}
// PID���ƺ���
// �޸ĺ�� PID_realize ����
// pid.c
float PID_realize(tPid *pid, float actual_val) {
    pid->actual_val = actual_val;
    // pid->target_val = (pid->target_val > 0.0f) ? pid->target_val : 0.0f; // Ŀ��ֵ����0
    pid->err = pid->target_val - pid->actual_val;

    
    // ����һ���Լ�飺Ŀ����ʵ���ٶȷ����෴ʱ���������
    if ((pid->target_val > 0.0f && pid->actual_val < -1.0f) || 
        (pid->target_val < 0.0f && pid->actual_val > 1.0f)) {
        pid->err_sum = 0.0f;  // ���������
    }
    // if (fabsf(pid->err) < pid->deadband) {
    //     pid->err = 0;
    // }
//    if(fabsf(actual_val) <= 0.1f && pid->err_sum != 0)
//    {
//        pid->err_sum *= 0.9f;
//    }
    // �������ۼӣ����޷���
    pid->err_sum += pid->err * pid->Ki;
    if (pid->err_sum > pid->max_integral) {
        pid->err_sum = pid->max_integral;
    } else if (pid->err_sum < -pid->max_integral) {
        pid->err_sum = -pid->max_integral;
    }
    // if ((pid->err_last > 0 && pid->err < 0) || (pid->err_last < 0 && pid->err > 0)) {
    //     pid->err_sum = 0;  // ���������仯ʱ������֣��������
    // }
    // ΢������㣨���޷���
    float d_err = pid->err - pid->err_last;
    if (d_err > pid->max_derivative) {
        d_err = pid->max_derivative;
    } else if (d_err < -pid->max_derivative) {
        d_err = -pid->max_derivative;
    }
    float derivative = d_err * pid->Kd;

    // ����PID���
    float output = pid->Kp * pid->err + pid->err_sum + derivative;

    // float delta = output - pid->last_output;
    // float max_delta = pid->max_delta;  // ������
    // if (delta > max_delta) delta = max_delta;
    // if (delta < -max_delta) delta = -max_delta;
    // output = pid->last_output + delta;

    // ����޷�
    if (output > pid->max_output) 
    {
        output = pid->max_output;
    } 
    else if (output < pid->min_output) 
    {
        output = pid->min_output;
    }
    
    // if (fabsf(output) <= pid->deadband)
    // {
    //     output = 0.0f;
    // }
    

    // ���������ʷ
    pid->err_last = pid->err;
    return output;
}


float PID_Angle(tPid *pid, float actual_yaw, float target_yaw) {
    const float ANGLE_DT = 0.004f; // 与你在中断里用的 0.004s 保持一致
    float err = target_yaw - actual_yaw;
    pid->target_val = target_yaw;
    pid->actual_val = actual_yaw;
    // 归一到 [-180,180]
    while (err > 180.0f) err -= 360.0f;
    while (err < -180.0f) err += 360.0f;

    pid->err = err;

    // 积分死区：较小阈值，避免完全抑制积分
    const float INTEGRAL_DEADBAND = 0.2f; // 0.2度以内不积累
    if (fabsf(err) < INTEGRAL_DEADBAND) {
        // 不清零 err_sum，改为轻微衰减，避免突变后积分风up
        pid->err_sum *= 0.98f;
    } else {
        // 按 dt 累积误差，再在输出时乘 Ki
        pid->err_sum += err * ANGLE_DT;
    }

    // 积分限幅（err_sum 为原始积分和，Ki 在输出时乘）
    if (pid->err_sum > pid->max_integral) pid->err_sum = pid->max_integral;
    if (pid->err_sum < -pid->max_integral) pid->err_sum = -pid->max_integral;

    // 导数（按 dt）
    float derivative = (err - pid->err_last) / ANGLE_DT;

    // PID 输出（注意 Ki 乘到积分和上）
    float raw_output = pid->Kp * err + pid->Ki * pid->err_sum + pid->Kd * derivative;

    // 速率限制：允许更大的默认变化
    float max_delta = (pid->max_delta > 0.0f) ? pid->max_delta : 0.2f; // 放宽为 0.2 每周期
    float delta = raw_output - pid->last_output;
    if (delta > max_delta) delta = max_delta;
    if (delta < -max_delta) delta = -max_delta;
    float output = pid->last_output + delta;

    // 输出限幅
    if (output > pid->max_output) output = pid->max_output;
    if (output < -pid->max_output) output = -pid->max_output;

    // 更新历史
    pid->err_last = err;
    pid->last_output = output;

    return output;
}
