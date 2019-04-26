/*
 * ring_buffer.c
 *
 *  Created on: Jan 5, 2019
 *      Author: ali-teke
 */

#include <string.h>
#include "stm32f0xx_hal.h"
#include "ring_buffer.h"

void init_ring_buffer(struct ring_buffer *buffer, void *storage, int size)
{
	buffer->end_pos = 0;
	buffer->start_pos = 0;
	buffer->storage = storage;
	buffer->size = size;
}

void ring_buffer_write(struct ring_buffer *buffer, const void *data, int data_length)
{
	int start_pos, copy_length, free_size;

	start_pos = buffer->start_pos;
	if (start_pos > buffer->end_pos)
		free_size = start_pos - buffer->end_pos - 1;
	else
		free_size = buffer->size - buffer->end_pos + buffer->start_pos - 1;

	copy_length = (data_length < free_size) ? data_length : free_size;
	if (buffer->end_pos + copy_length <= buffer->size)
	{
		memcpy((uint8_t*)buffer->storage + buffer->end_pos, data, copy_length);
		buffer->end_pos += copy_length;
	}
	else
	{
		size_t size;

		size = buffer->size - buffer->end_pos;
		memcpy((uint8_t*)buffer->storage + buffer->end_pos, data, size);
		memcpy(buffer->storage, (const uint8_t*)data + size, copy_length - size);
		buffer->end_pos = copy_length - size;
	}
}
