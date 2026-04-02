/*
 * ring_buffer.c
 *
 *  Created on: 31-Mar-2026
 *      Author: Parth
 */

#include <string.h>
#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t *rb){
	rb->head = 0;
	rb->tail = 0;
	rb->count = 0;
	memset(rb->buffer, 0, sizeof(rb->buffer));
}

void RingBuffer_Write(RingBuffer_t *rb, DataPoint_t *data){
	rb->buffer[rb->head] = *data;
	rb->head = (rb->head + 1) % RING_BUFFER_SIZE;

	if(rb->count == RING_BUFFER_SIZE){
		rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
	}else{
		rb->count++;
	}
}

uint8_t RingBuffer_Read(RingBuffer_t *rb, uint8_t index, DataPoint_t *data){
	if(index >= rb->count){
		return 0;
	}

	uint8_t actual_index = (rb->tail + index) % RING_BUFFER_SIZE;
	*data = rb->buffer[actual_index];
	return 1;
}

uint8_t RingBuffer_IsFull(RingBuffer_t *rb){
	return rb->count == RING_BUFFER_SIZE;
}

uint8_t RingBuffer_IsEmpty(RingBuffer_t *rb){
	return rb->count == 0;
}

uint8_t RingBuffer_GetCount(RingBuffer_t *rb){
	return rb->count;
}

