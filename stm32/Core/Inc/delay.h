/*
 * delay.h
 *
 *  Created on: Jan 20, 2025
 *      Author: 52335
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#include "stdint.h"
#include "main.h"
#include "dht11.h"
#include "tim.h"
void delay_us(uint16_t delay);
void delay_ms(uint16_t delay);
void delay_s(uint16_t delay);


#endif /* INC_DELAY_H_ */
