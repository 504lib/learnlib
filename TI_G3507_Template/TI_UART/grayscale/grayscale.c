#include "grayscale.h"
#include "./PID_Node/PID_Node.h"


float CalculateGrayError_Advanced(uint8_t gray_byte)
{
  static const float symmetric_weights[GRAY_BITS] = {
         3.5f,  // λ0
         2.5f,  // λ1
         1.5f,  // λ2
         0.5f,  // λ3
        -0.5f,  // λ4
        -1.5f,  // λ5 
        -2.5f,  // λ6
        -3.5f   // λ7
  };
        //  1.0f,  // λ0
        //  0.8f,  // λ1
        //  0.4f,  // λ2
        //  0.0f,  // λ3
        // -0.0f,  // λ4
        // -0.4f,  // λ5 
        // -0.8f,  // λ6
        // -1.0f   // λ7
        
        //  3.5f,  // λ0
        //  2.5f,  // λ1
        //  1.5f,  // λ2
        //  0.5f,  // λ3
        // -0.5f,  // λ4
        // -1.5f,  // λ5 
        // -2.5f,  // λ6
        // -3.5f   // λ7
    // 1. ����ԳƼ�Ȩ��
    float symmetric_sum = 0.0f;
    int total_count = 0;
    
    for (int i = 0; i < GRAY_BITS; i++)
    {
        if ((gray_byte & (1 << i)) == 0)
        {
            symmetric_sum += symmetric_weights[i];
            total_count++;
        }
    }
    
    // 2. ���û�м��㣬����0
    if (total_count == 0 || total_count == GRAY_BITS)
        return 0.0f;
    
    // 3. ��һ��
    float normalized = symmetric_sum / total_count;
    
    // 4. Ӧ�÷����Ժ�����arctan������ƽ���ұ���
    // error = (2/��) * atan(k * normalized)������k����������
    // const float k = 3.0f;  // ������ϵ��
    // float error = 2.0f * atanf(k * normalized) / 3.14159265f;
    return normalized;
}
