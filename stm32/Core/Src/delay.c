#include "delay.h"
#include "stdint.h"
#include "main.h"
#include "dht11.h"
#include "tim.h"
/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */
void delay_us(uint16_t delay) {
    __HAL_TIM_DISABLE(&htim3);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_ENABLE(&htim3);
    uint16_t curCnt = 0;
    while (1) {
        curCnt = __HAL_TIM_GET_COUNTER(&htim3);
        if (curCnt >= delay)
            break;
    }
    __HAL_TIM_DISABLE(&htim3);
}


/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
void delay_ms(uint16_t delay)
{
	while(delay--)
	{
		delay_us(1000);
	}
}

/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void delay_s(uint16_t delay)
{
	while(delay--)
	{
		delay_ms(1000);
	}
}
