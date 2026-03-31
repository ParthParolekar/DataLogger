/*
 * utils.c
 *
 *  Created on: 21-Mar-2026
 *      Author: Lenovo
 */

#include "utils.h"

static TIM_HandleTypeDef *_htim = NULL;

void utils_init(TIM_HandleTypeDef *htim){
	_htim = htim;
	HAL_TIM_Base_Start(_htim);
}

void delay_us(uint16_t us){
	__HAL_TIM_SET_COUNTER(_htim, 0);
	while (__HAL_TIM_GET_COUNTER(_htim) < us);
}
