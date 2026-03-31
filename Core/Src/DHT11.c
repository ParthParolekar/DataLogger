/*
 * DHT11.c
 *
 *  Created on: 03-Feb-2026
 *      Author: Parth
 */

#include "DHT11.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"

extern TIM_HandleTypeDef htim14;

/************** Private Macros **************/
#define DHT11_PORT 		GPIOA
#define DHT11_PIN		GPIO_PIN_1

/************** Private Function Prototypes **************/
static void DHT11_SetOutput(void);
static void DHT11_SetInput(void);

/************** Public Functions **************/

void DHT11_Init(void){
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

void DHT11_Start(void){
	DHT11_SetOutput();
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
	delay_us(18000);
	DHT11_SetInput();
	delay_us(40);
}

DHT11_Status_t DHT11_CheckResponse(void){
	uint32_t timeout = 0;

	//Wait for DHT to pull line low (error if takes longer than 100us)
	timeout = 0;
	while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
		delay_us(1);
		timeout++;
		if(timeout > 100){
			return DHT11_ERROR_NO_RESPONSE;
		}
	}

	//Check if pulls high after 80us (100us to check for error)
	timeout = 0;
	while(!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))){
		delay_us(1);
		timeout++;
		if(timeout > 100){
			return DHT11_ERROR_TIMEOUT_LOW;
		}
	}

	//Check if pulls low again after 80us (100us to check for error)
	timeout = 0;
	while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
		delay_us(1);
		timeout++;
		if(timeout > 100){
			return DHT11_ERROR_TIMEOUT_HIGH;
		}
	}

	return DHT11_OK;
}

//DHT11_Status_t DHT11_ReadBit(uint8_t *bit){
//	uint32_t timeout = 0;
//	uint8_t threshold = 50;
//	uint32_t pulse_width = 0;
//
//
//	while(!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))){
//		delay_us(1);
//		timeout++;
//		if(timeout > 100){
//			return DHT11_BIT_TIMEOUT_ERROR;
//		}
//	}
//
//	pulse_width = 0;
//	char uart_buf[64];
//
//	while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
//		delay_us(1);
//		pulse_width++;
//		t =  __HAL_TIM_GET_COUNTER(&htim14);
//		if(pulse_width > 100){
//			return DHT11_BIT_TIMEOUT_ERROR;
//		}
//	}

//
//	if(pulse_width < threshold){
//		*bit = 0;
//	}else{
//		*bit = 1;
//	}
//
//	return DHT11_OK;
//
//}

DHT11_Status_t DHT11_ReadBit(uint8_t *bit)
{
    uint32_t timeout = 0;
    uint32_t pulse_width;
//    char uart_buf[64];

    /* 1) Wait for LOW (bit start) */
//    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
//        delay_us(1);
//        if (++timeout > 100) {
//            return DHT11_BIT_TIMEOUT_ERROR;
//        }
//    }

    /* 2) Wait for HIGH */
    timeout = 0;
    while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        delay_us(1);
        if (++timeout > 100) {
            return DHT11_BIT_TIMEOUT_ERROR;
        }
    }

    /* 3) Measure HIGH pulse width */
    __HAL_TIM_SET_COUNTER(&htim14, 0);

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        if (__HAL_TIM_GET_COUNTER(&htim14) > 100) {
            return DHT11_BIT_TIMEOUT_ERROR;
        }
    }

    pulse_width = __HAL_TIM_GET_COUNTER(&htim14);

    /* 4) Decode bit */
    *bit = (pulse_width > 50) ? 1 : 0;

    /* 5) Wait for post-bit LOW (separator) */
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        delay_us(1);
        if (++timeout > 100) {
            return DHT11_BIT_TIMEOUT_ERROR;
        }
    }

    return DHT11_OK;
}

DHT11_Status_t DHT11_ReadByte(uint8_t *byte){
	uint8_t bit = 0;

	*byte = 0;

	for(uint8_t i = 0; i<8; i++){

		DHT11_Status_t Read_Bit_Status = DHT11_ReadBit(&bit);

		if(Read_Bit_Status != DHT11_OK){
			return Read_Bit_Status;
		}

		if(bit == 0){
			*byte &= ~(1 << (7-i));
		}

		if(bit == 1){
			*byte |= (1 << (7-i));
		}

	}

	return DHT11_OK;
}

DHT11_Status_t DHT11_ReadData(DHT11_Data_t *data){

	uint8_t RawData[5];
	uint8_t byte = 0;
	uint8_t checksum;

	//1.) Send Start Signal
	DHT11_Start();

	//2.) Check Response
	DHT11_Status_t response = DHT11_CheckResponse();

	if(response != DHT11_OK){
		return response;
	}



	//3) Read 5 bytes of data and store
	for(uint8_t i = 0; i < 5; i++){

		DHT11_Status_t ByteDataResponse = DHT11_ReadByte(&byte);

		if(ByteDataResponse != DHT11_OK){
			return ByteDataResponse;
		}

		RawData[i] = byte;
		byte = 0;
	}


	//4) Calculate Checksum
	checksum = RawData[0] + RawData[1] + RawData[2] + RawData[3];
	checksum &= 0xFF;

	//5) Validate Checksum
	if(checksum != RawData[4]){
		return DHT11_CHECKSUM_ERROR;
	}

	data->humidity = RawData[0];
	data->temperature = RawData[2];

	return DHT11_OK;
}


/************** Private Helper Functions **************/
static void DHT11_SetOutput(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_SetInput(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}



