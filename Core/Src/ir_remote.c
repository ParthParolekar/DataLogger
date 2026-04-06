/*
 * ir_remote.c
 *
 *  Created on: 03-Apr-2026
 *      Author: Parth
 */
#include "main.h"
#include "ir_remote.h"
#include "utils.h"

#define IR_INPUT_PORT			IR_INPUT_GPIO_Port
#define IR_INPUT_PIN			IR_INPUT_Pin

#define LEADER_LOW_US			9000
#define LEADER_HIGH_US			4500
#define BIT_LOW_US				562
#define BIT_0_HIGH_US			562
#define BIT_1_HIGH_US			1687
#define IR_THRESHOLD_US			1700
#define REPEAT_HIGH_US			2250

static TIM_HandleTypeDef *_htim;

static IR_State_t _state = IR_STATE_IDLE;
static uint32_t _raw_data = 0;
static uint8_t _bit_count = 0;
static IR_Data_t _ir_data = {0};

void IR_Init(TIM_HandleTypeDef *htim){
	_htim = htim;
	HAL_TIM_Base_Start(_htim);
}

void IR_EXTI_Callback(){
	//1)Read how long the pin was high
	uint32_t pulse = __HAL_TIM_GET_COUNTER(_htim);

	//2) Immediately reset the counter
	__HAL_TIM_SET_COUNTER(_htim, 0);

	//3 State Machine
	switch(_state){

	case IR_STATE_IDLE:
		if(pulse > 12000 && pulse < 15000){
			_state = IR_STATE_DATA;
			_raw_data = 0;
			_bit_count = 0;
		}else if(pulse > 2000 && pulse < 2500){
			_ir_data.repeat = 1;
		}
		break;

	case IR_STATE_DATA:
		if(pulse < 300){
			//garbage - pulse too short
			_state = IR_STATE_IDLE;
			break;
		}

		_raw_data = _raw_data >> 1; //Shift data to right. LSB First

		if(pulse > IR_THRESHOLD_US){
			_raw_data = _raw_data | (1 << 31); //Put 1 at MSB and shift to right in next iteration

			//No need to check for 0 bit, already took care when right sifted
		}

		_bit_count++;

		if(_bit_count == 32){
			uint8_t addr 		= 	_raw_data 			& 0xFF;
			uint8_t addr_inv 	= 	(_raw_data >> 8) 	& 0xFF;
			uint8_t cmd 		= 	(_raw_data >> 16) 	& 0xFF;
			uint8_t cmd_inv 	= 	(_raw_data >> 24) 	& 0xFF;

			if((addr + addr_inv == 0xFF) && (cmd + cmd_inv == 0xFF)){
				_ir_data.address = addr;
				_ir_data.command = cmd;
				_ir_data.data_ready = 1;
				_ir_data.repeat = 0;
			}

			_state = IR_STATE_IDLE;
		}
		break;

	}
}

uint8_t IR_Get_Command(IR_Data_t *data){
	if(_ir_data.data_ready == 1){
		*data = _ir_data;
		_ir_data.data_ready = 0;
		return 1;
	}

	return 0;
}
