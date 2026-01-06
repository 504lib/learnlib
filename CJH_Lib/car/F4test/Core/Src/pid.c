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
    pidMotor1Speed.target_val = 0.0;
    pidMotor1Speed.err = 0.0;
    pidMotor1Speed.err_last = 0.0;
    pidMotor1Speed.err_sum = 0.0;
    pidMotor1Speed.Kp = 10.0;
    pidMotor1Speed.Ki = 0.0;
    pidMotor1Speed.Kd = 0.0;
    pidMotor1Speed.max_integral = 100.0;  // �����޷�
    pidMotor1Speed.max_derivative = 20.0;
    pidMotor1Speed.max_output = 150.0;    // ����޷�
    pidMotor1Speed.min_output =0;
    pidMotor1Speed.last_output = 0.0;
    pidMotor1Speed.max_delta = 20.0;      // ÿ�����仯��
    pidMotor1Speed.deadband = 0.5;        // ��������ѡ��

    // ���2 PID��ʼ����ͬ�ϣ�
    pidMotor2Speed.actual_val = 0.0;
    pidMotor2Speed.target_val = 0.0;
    pidMotor2Speed.err = 0.0;
    pidMotor2Speed.err_last = 0.0;
    pidMotor2Speed.err_sum = 0.0;
    pidMotor2Speed.Kp = 10.0;
    pidMotor2Speed.Ki = 0.0;
    pidMotor2Speed.Kd = 0.0;
    pidMotor2Speed.max_integral = 100.0;
    pidMotor2Speed.max_derivative = 20.0;
    pidMotor2Speed.max_output = 100.0;
    pidMotor2Speed.min_output = 0;
    pidMotor2Speed.last_output = 0.0;
    pidMotor2Speed.max_delta = 20.0;      // ÿ�����仯��
    pidMotor2Speed.deadband = 0.5;        // ��������ѡ��

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
    pidMpu6050.max_delta = 0.0;      // ÿ�����仯��
    pidMpu6050.deadband = 0.0;        // ��������ѡ��

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
    pid_yaw.max_delta = 0.0;      // ÿ�����仯��
    pid_yaw.deadband = 0.0;        // ��������ѡ��

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
    pid->err = pid->target_val - pid->actual_val;

    // ����һ���Լ�飺Ŀ����ʵ���ٶȷ����෴ʱ���������
    if ((pid->target_val > 0 && pid->actual_val < -1.0) || 
        (pid->target_val < 0 && pid->actual_val > 1.0)) {
        pid->err_sum = 0;  // ���������
    }
    // if (fabsf(pid->err) < pid->deadband) {
    //     pid->err = 0;
    // }
    if(actual_val == 0 && pid->err_sum != 0)
    {
        pid->err_sum *= 0.9;
    }
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

    // ���������ʷ
    pid->err_last = pid->err;
    return output;
}


float PID_Angle(tPid *pid, float actual_yaw, float target_yaw) {
    float err = target_yaw - actual_yaw;
    pid->target_val = target_yaw;  // ����Ŀ��ֵ
    pid->actual_val = actual_yaw;
    // �������Ϊ [-180, 180]
    while (err > 180) err -= 360;
    while (err < -180) err += 360;

    pid->err = err;

    // ����������ԣ��������ű任��̫Сʱ���㣩
    if (fabsf(err) < 1.0f || (pid->err * pid->err_last < 0)) {
        pid->err_sum = 0;
    } else {
        pid->err_sum += pid->Ki * err;
    }

    // ���ƻ���
    if (pid->err_sum > pid->max_integral) pid->err_sum = pid->max_integral;
    if (pid->err_sum < -pid->max_integral) pid->err_sum = -pid->max_integral;

    // �������
    float output = pid->Kp * err + pid->err_sum + pid->Kd * (err - pid->err_last);
    pid->err_last = err;

    // �������
    if (output > pid->max_output) output = pid->max_output;
    if (output < -pid->max_output) output = -pid->max_output;

    return output;
}

