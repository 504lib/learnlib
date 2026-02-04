#include "hx711.h"

static int32_t offset = 0;  // 去皮值

#define HX711_OFFSET     8500000L    // 零点偏移值（无负载时的AD值）
#define HX711_SCALE      0.00025f    // 比例系数（AD值到克的转换系数）
#define FILTER_WINDOW_SIZE 5         // 滤波窗口大小

typedef struct {
    int32_t offset;
    float scale;
    float filter_buffer[FILTER_WINDOW_SIZE];
    uint8_t filter_index;
    uint8_t filter_count;
} HX711_Weight_t;

static HX711_Weight_t weight_data = {
    .offset = HX711_OFFSET,
    .scale = HX711_SCALE,
    .filter_index = 0,
    .filter_count = 0
};

// 读取HX711原始值
static int32_t HX711_Read(void)
{
    int32_t value = 0;
    
    // 等待数据准备好，带超时保护
    uint32_t timeout = 1000;  // 10ms超时
    static uint32_t last_tick = 0;
    last_tick = Get_Tick();
    while (HAL_GPIO_ReadPin(HX711_DOUT_GPIO_Port, HX711_DOUT_Pin) == GPIO_PIN_SET)
    {
        if(Get_Tick() - last_tick >= timeout) return 0;  // 超时返回
    }
    
    // 禁用中断确保时序精确
    _disable_interrupt_func;
    
    for (uint8_t i = 0; i < 24; i++)
    {
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
        
        // 精确延时
        last_tick = Get_Tick();
        while ((Get_Tick() - last_tick >= 10) )
        {
            break;
        }
        
        
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
        
        // 在时钟下降沿后读取
        last_tick = Get_Tick();
        while ((Get_Tick() - last_tick >= 10) )
        {
            break;
        }
        
        value = value << 1;
        if (HAL_GPIO_ReadPin(HX711_DOUT_GPIO_Port, HX711_DOUT_Pin) == GPIO_PIN_SET)
        {
            value++;
        }
    }
    
    // 第25个脉冲设置增益
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
    last_tick = Get_Tick();
    while ((Get_Tick() - last_tick >= 10) )
    {
            break;
    }
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    
    // 重新启用中断
    _enable_interrupt_func;
    
    // 符号扩展
    if (value & 0x800000)
    {
        value |= 0xFF000000;
    }
    
    return value;
}

// 去皮（清零）函数
void HX711_Tare(void)
{
    int32_t sum = 0;
    uint32_t last_tick = 0;
    // 读取10次取平均值
    for (uint8_t i = 0; i < 10; i++)
    {
        sum += HX711_Read();
        last_tick = Get_Tick();
        while ((Get_Tick() - last_tick >= 10))
        {
            break;
        }
    }
    
    weight_data.offset = sum / 10;
    LOG_INFO("offset:%d",weight_data.offset);
}

// 获取重量值（去皮后）
int32_t HX711_GetWeight(void)
{
    return HX711_Read() - offset;
}


/**
 * @brief 获取滤波后的重量值
 * @return 重量值（单位：克）
 * @note 使用移动窗口滤波算法，自动处理去皮和单位转换
 */
float HX711_GetFilteredWeight(void)
{
    static float filtered_weight = 0;
    float current_weight;
    int32_t raw_value;
    
    // 读取原始AD值
    raw_value = HX711_GetWeight();
    
    // 转换为重量：重量 = (原始值 - 偏移量) × 比例系数
    current_weight = (raw_value - weight_data.offset) * weight_data.scale;
    
    // 窗口滤波处理
    weight_data.filter_buffer[weight_data.filter_index] = current_weight;
    weight_data.filter_index = (weight_data.filter_index + 1) % FILTER_WINDOW_SIZE;
    
    if (weight_data.filter_count < FILTER_WINDOW_SIZE)
    {
        weight_data.filter_count++;
    }
    
    // 计算窗口平均值
    float sum = 0;
    for (uint8_t i = 0; i < weight_data.filter_count; i++)
    {
        sum += weight_data.filter_buffer[i];
    }
    
    filtered_weight = sum / weight_data.filter_count;
    
    return filtered_weight;
}

