#ifndef DHT11_H
#define DHT11_H

#include "main.h"

void Dht11_DATA_OUT(void);                       // 设置数据交互口为输出
void Dht11_DATA_IN(void);                        // 设置数据交互口为输入
void DHT11_Rst(void);                            // 复位 DHT11
uint8_t DHT11_Check(void);                       // 检测 DHT11 状态
uint8_t DHT11_Read_Bit(void);                    // 读取一位数据
uint8_t DHT11_Read_Byte(void);                   // 读取一字节数据
uint8_t DHT11_Read_Data(uint8_t *humi, uint8_t *temp); // 读取温湿度数据

#endif // DHT11_H
