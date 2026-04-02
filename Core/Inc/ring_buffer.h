/*
 * ring_buffer.h
 *
 *  Created on: 31-Mar-2026
 *      Author: Parth
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include "stm32g0xx_hal.h"

#define RING_BUFFER_SIZE			64

typedef struct {
	uint8_t temperature;
	uint8_t humidity;
	uint16_t distance;
	uint32_t timestamp;
} DataPoint_t;

typedef struct {
	DataPoint_t buffer[RING_BUFFER_SIZE];
	uint8_t head;
	uint8_t tail;
	uint8_t count;
} RingBuffer_t;


void RingBuffer_Init(RingBuffer_t *rb);
void RingBuffer_Write(RingBuffer_t *rb, DataPoint_t *data);
uint8_t RingBuffer_Read(RingBuffer_t *rb, uint8_t index, DataPoint_t *data);
uint8_t RingBuffer_IsFull(RingBuffer_t *rb);
uint8_t RingBuffer_IsEmpty(RingBuffer_t *rb);
uint8_t RingBuffer_GetCount(RingBuffer_t *rb);

#endif /* INC_RING_BUFFER_H_ */
