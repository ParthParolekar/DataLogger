/*
 * utils.h
 *
 *  Created on: 21-Mar-2026
 *      Author: Lenovo
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include "stm32g0xx_hal.h"

void utils_init(TIM_HandleTypeDef *htim);
void delay_us(uint16_t us);



#endif /* INC_UTILS_H_ */
