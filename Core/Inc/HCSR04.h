/*
 * HCSR04.h
 *
 *  Created on: 29-Mar-2026
 *      Author: Lenovo
 */

#ifndef INC_HCSR04_H_
#define INC_HCSR04_H_

#include "stm32g0xx_hal.h"

typedef enum{
	HCSR04_OK = 0,
	HCSR04_NO_ECHO,
	HCSR04_ERR_TIMEOUT
} HCSR04_Status_t;

typedef struct {
	uint16_t distance_cm;
} HCSR04_Data_t;

void HCSR04_Init(TIM_HandleTypeDef *htim);
HCSR04_Status_t HCSR04_Read(HCSR04_Data_t *data);

#endif /* INC_HCSR04_H_ */
