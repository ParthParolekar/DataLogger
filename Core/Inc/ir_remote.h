/*
 * ir_remote.h
 *
 *  Created on: 03-Apr-2026
 *      Author: Parth
 */

#ifndef INC_IR_REMOTE_H_
#define INC_IR_REMOTE_H_

#include "stm32g0xx_hal.h"

typedef enum {
	IR_STATE_IDLE,
	IR_STATE_DATA
} IR_State_t;

typedef struct {
	uint8_t address;
	uint8_t command;
	uint8_t data_ready;
	uint8_t repeat;
}IR_Data_t;

void IR_Init(TIM_HandleTypeDef *htim);
void IR_EXTI_Callback(void);
uint8_t IR_Get_Command(IR_Data_t *data);

#endif /* INC_IR_REMOTE_H_ */
