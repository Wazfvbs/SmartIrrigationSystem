/*
 * sensors.c
 *
 *  Created on: Jan 31, 2025
 *      Author: 52335
 */

#include "sensors.h"
#include "adc.h"      // 用到了 hadc1
#include "delay.h"    // 如果你有自己写的 delay_us 等，需要包含
#include "dht11.h"    // DHT11 的读写函数

/**
  * @brief  这里可根据需要做一些一次性的初始化操作
  *         比如启动ADC、启动定时器等。
  *         如果全部都由Cube自动生成并在main.c里初始化了，这里可以留空。
  */
void SENSORS_Init(void)
{
    // 一般在 main.c 中已经调用了：
    //   MX_ADC1_Init();
    //   MX_TIM3_Init();
    //   HAL_TIM_Base_Start(&htim3);    // 如果要用TIM3来做us延时
    // 所以这里可以什么都不做，也可以做一些校准类的操作

    // 如果你想做ADC的校准，可以加（F1里一般是自动做了）：
    // HAL_ADCEx_Calibration_Start(&hadc1);

    // DHT11 初始化：实际也不需要太多，你的 dht11.c 里主要是读写过程
}

/**
  * @brief  读取指定 ADC 通道的数值（单次转换），返回 0~4095
  * @param  channel: 例如 ADC_CHANNEL_5, ADC_CHANNEL_1 等
  */




static uint16_t SENSORS_ReadADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;                               // 选择通道
    sConfig.Rank = ADC_REGULAR_RANK_1;                       // 通道序列
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;       // 采样时间
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        // 若想处理错误，可在这里加断言或直接返回某个错误值
        return 0;
    }

    // 启动转换
    HAL_ADC_Start(&hadc1);

    // 等待转换完成 (超时10ms)
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        // 读取结果
        uint16_t val = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        return val;
    }
    HAL_ADC_Stop(&hadc1);
    // 若超时，则返回0或你想要的错误值
    return 0;
}


uint16_t GetHumidity(int times)
{
    uint32_t H_all = 0;
    float H_arg = 0;
    uint8_t t;

    // 进行多次ADC转换并累加
    for (t = 0; t < times; t++) {
        H_all += SENSORS_ReadADC_Channel(ADC_CHANNEL_5);
        delay_ms(1); // 延时1毫秒
    }
    // 计算平均值
    H_arg = (H_all / times);
    // 根据转换公式计算湿度值
    uint16_t data = (4095 - H_arg) / 3292 * 100;
    // 确保湿度值在合理范围内（0~100）
    data = data > 100 ? 100 : (data < 0 ? 0 : data);
    return data;
}

uint16_t GetLightValue(int times)
{
    uint32_t sum = 0;
    for (int i = 0; i < times; i++)
    {
        sum += SENSORS_ReadADC_Channel(ADC_CHANNEL_2);  // 这里就是读 PA2
        delay_ms(1);
    }
    return (uint16_t)(sum / times);
}

/**
  * @brief  读取全部传感器
  *         1. DHT11: 数字接口
  *         2. 土壤湿度: 例如 PA5 => ADC_CHANNEL_5
  *         3. 水位: 例如 PA1 => ADC_CHANNEL_1
  */
void SENSORS_ReadAll(Sensor_DataTypeDef *data)
{
    // 1. 读取 DHT11
    // 如果 DHT11_Read_Data() 返回 0 表示成功
    if (DHT11_Read_Data(&data->humidity, &data->temperature) != 0)
    {
        // 读取失败时，可考虑赋默认值
        data->temperature = 0xFF;
        data->humidity = 0xFF;
    }

    // 2. 读取土壤湿度 - channel 5



    data->soilADC = GetHumidity(10);
    uint8_t Depth=0;
    uint16_t ADC_Depth;
    // 3. 读取水位 - channel 1
    ADC_Depth=(float)SENSORS_ReadADC_Channel(ADC_CHANNEL_1)/4096*3.3;
    if((ADC_Depth-1500)>0){
    	Depth=(ADC_Depth-1550)/100;
	}
    else
	{
		Depth=0;
	}
    data->waterADC = SENSORS_ReadADC_Channel(ADC_CHANNEL_1);

    data->lightADC = GetLightValue(10);
}
