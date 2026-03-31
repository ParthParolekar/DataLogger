/*
 * HCSR04.c
 *
 *  Created on: 29-Mar-2026
 *      Author: Parth
 */

#include "main.h"
#include "utils.h"
#include "HCSR04.h"


#define TRIG_PORT			HCSR04_TRIG_GPIO_Port
#define TRIG_PIN			HCSR04_TRIG_Pin
#define ECHO_PORT			HCSR04_ECHO_GPIO_Port
#define ECHO_PIN			HCSR04_ECHO_Pin

#define ECHO_TIMEOUT_US		38000
#define TRIG_PULSE_US		10

static TIM_HandleTypeDef *_htim;

void HCSR04_Init(TIM_HandleTypeDef *htim){
	_htim = htim;
	HAL_TIM_Base_Start(_htim);
	HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}

HCSR04_Status_t HCSR04_Read(HCSR04_Data_t *data){

	uint32_t pulse_width = 0;

	//1) Send 10us trigger pulse
	HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

	//2) Wait for Echo to go High
	__HAL_TIM_SET_COUNTER(_htim, 0);
	while(!HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)){
		if(__HAL_TIM_GET_COUNTER(_htim) > ECHO_TIMEOUT_US){
			return HCSR04_NO_ECHO;
		}
	}

	//3) Measure for how long Echo stays high
	__HAL_TIM_SET_COUNTER(_htim, 0);
	while(HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)){
		if(__HAL_TIM_GET_COUNTER(_htim) > ECHO_TIMEOUT_US){
			return HCSR04_NO_ECHO;
		}
	}

	pulse_width = __HAL_TIM_GET_COUNTER(_htim);

	//4) Convert to cm
	//Speed of sound = 343 m/s = 0.0343 cm/us
	//Distance = (pulse_width * 0.0343) / 2 = pulse_width
	data->distance_cm = (pulse_width * 0.0343) / 2;

	return HCSR04_OK;
}
