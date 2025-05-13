/*
 * sensors.h
 *
 *  Created on: Jan 31, 2025
 *      Author: 52335
 */

#ifndef INC_SENSORS_H_
#define INC_SENSORS_H_

#include "main.h"   // 里边会包含stm32f1xx_hal.h等
#include <stdint.h>

// 这里可定义一个结构体，把多个传感器的结果都保存起来
typedef struct
{
    uint8_t temperature; // DHT11温度
    uint8_t humidity;    // DHT11湿度
    uint16_t soilADC;    // 土壤湿度的ADC值
    uint16_t waterADC;   // 水位的ADC值
    uint16_t lightADC;    // 光敏电阻的ADC值
} Sensor_DataTypeDef;

/* 初始化所有传感器（如果需要的话） */
void SENSORS_Init(void);

/* 读取所有传感器的数据，并存入传入的 data 结构 */
void SENSORS_ReadAll(Sensor_DataTypeDef *data);

#endif /* INC_SENSORS_H_ */
