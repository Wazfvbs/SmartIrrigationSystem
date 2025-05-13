#include "dht11.h"
#include "tim.h"
#include "delay.h"


void Dht11_DATA_OUT(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Dht11_DATA_IN(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void DHT11_Rst(void) {
    Dht11_DATA_OUT();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    delay_us(30);
}

uint8_t DHT11_Check(void) {
    uint8_t retry = 0;
    Dht11_DATA_IN();
    while (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) && retry < 200) {
        retry++;
        delay_us(1);
    }
    if (retry >= 100) return 1;

    retry = 0;
    while (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) && retry < 200) {
        retry++;
        delay_us(1);
    }
    if (retry >= 100) return 1;
    return 0;
}

uint8_t DHT11_Read_Bit(void) {
    uint8_t retry = 0;
    while (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) && retry < 100) {
        retry++;
        delay_us(1);
    }
    retry = 0;
    while (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) && retry < 100) {
        retry++;
        delay_us(1);
    }
    delay_us(40);

    return GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
}

uint8_t DHT11_Read_Byte(void) {
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++) {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

uint8_t DHT11_Read_Data(uint8_t *humi, uint8_t *temp) {
    uint8_t buf[5];
    DHT11_Rst();
    if (DHT11_Check() == 0) {
        for (uint8_t i = 0; i < 5; i++) buf[i] = DHT11_Read_Byte();
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4]) {
            *humi = buf[0];
            *temp = buf[2];
        }
    } else {
        return 1;
    }
    return 0;
}
