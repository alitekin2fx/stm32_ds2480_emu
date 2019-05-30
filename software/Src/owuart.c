/*
 * owuart.c
 *
 *  Created on: Apr 7, 2019
 *      Author: ali-teke
 *  https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
 *  https://www.maximintegrated.com/en/app-notes/index.mvp/id/214
 */

#include "stm32f0xx_hal.h"
#include "owuart.h"

extern uint32_t led1_tick_count;
extern uint32_t led2_tick_count;
extern UART_HandleTypeDef huart2;

static void owuart_set_baudrate(int baudrate);

static int owuart_convert_to_bit(uint8_t data);
static uint8_t owuart_convert_from_bit(int bit_value);

static uint8_t owuart_convert_to_byte(const uint8_t *data);
static void owuart_convert_from_byte(uint8_t byte_value, uint8_t *data);

static int owuart_touch_data(uint8_t *tx_data, uint8_t *rx_data, int bit_count);

static const uint8_t owcrc8_table[] =
{
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

void owuart_set_baudrate(int baudrate)
{
	__HAL_UART_DISABLE(&huart2);

	huart2.Init.BaudRate = baudrate;
	HAL_UART_Init(&huart2);

	__HAL_UART_ENABLE(&huart2);
}

int owuart_convert_to_bit(uint8_t data)
{
	return(data == 0xff);
}

uint8_t owuart_convert_from_bit(int bit_value)
{
	return(bit_value ? 0xff : 0x00);
}

uint8_t owuart_convert_to_byte(const uint8_t *data)
{
	int i;
	uint8_t byte_value;

	for(i = 0, byte_value = 0; i < 8; i++)
	{
		if (owuart_convert_to_bit(data[i]))
			byte_value |= (1 << i);
	}
	return(byte_value);
}

void owuart_convert_from_byte(uint8_t byte_value, uint8_t *data)
{
	for(int i = 0; i < 8; i++)
		data[i] = owuart_convert_from_bit(byte_value & (1 << i));
}

int owuart_touch_data(uint8_t *tx_data, uint8_t *rx_data, int bit_count)
{
	int index;

	if (HAL_UART_Receive_DMA(&huart2, rx_data, bit_count) != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);

	if (HAL_UART_Transmit_DMA(&huart2, tx_data, bit_count) != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);

	/* Turn on LED1 */
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	led1_tick_count = HAL_GetTick();

	while(huart2.gState != HAL_UART_STATE_READY);
	while(huart2.RxState != HAL_UART_STATE_READY);

	/* We can't receive any character when OW pin is grounded */
	if (__HAL_DMA_GET_COUNTER(huart2.hdmarx) != 0)
		return(0);

	for(index = 0; index < bit_count; index++)
	{
		if (tx_data[index] != rx_data[index])
		{
			/* Turn on LED2 */
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
			led2_tick_count = HAL_GetTick();
		}
	}
	return(1);
}

int owuart_touch_reset()
{
	int res;
	uint8_t tx_data = 0xf0, rx_data;

	owuart_set_baudrate(9600);
	if (!owuart_touch_data(&tx_data, &rx_data, 1))
		res = 0;
	else
		res = (rx_data == 0xf0) ? 1 : 2;

	owuart_set_baudrate(115200);
	return(res);
}

int owuart_touch_bit(int tx_bit, int *rx_bit)
{
	uint8_t tx_data, rx_data;

	tx_data = owuart_convert_from_bit(tx_bit);
	if (!owuart_touch_data(&tx_data, &rx_data, 1))
		return(0);

	if (rx_bit != 0)
		*rx_bit = owuart_convert_to_bit(rx_data);

	return(1);
}

int owuart_touch_byte(uint8_t tx_byte, uint8_t *rx_byte)
{
	uint8_t tx_data[8], rx_data[8];

	owuart_convert_from_byte(tx_byte, tx_data);
	if (!owuart_touch_data(tx_data, rx_data, 8))
		return(0);

	if (rx_byte != 0)
		*rx_byte = owuart_convert_to_byte(rx_data);

	return(1);
}

int owuart_search(struct owuart_search_param *param)
{
	int id_bit, id_bit_number, cmp_id_bit;
	int last_zero, rom_byte_number, search_result;
	uint8_t crc8, rom_byte_mask, search_direction;

// Initialize for search
	crc8 = 0;
	last_zero = 0;
	id_bit_number = 1;
	rom_byte_mask = 1;
	search_result = 0;
	rom_byte_number = 0;

// If the last call was not the last one
	if (!param->last_device_flag)
	{
// 1-Wire reset
		if (owuart_touch_reset() != 2)
		{
// Reset the search
			param->last_device_flag = 0;
			param->last_discrepancy = 0;
			param->last_family_discrepancy = 0;
			return(0);
		}

// Issue the search command
		if (!owuart_touch_byte(0xf0, 0))
			return(0);

// Loop to do the search
		do
		{
// Read a bit and its complement
			if (!owuart_touch_bit(1, &id_bit) || !owuart_touch_bit(1, &cmp_id_bit))
				return(0);

// Check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
// All devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
				{
// Bit write value for search
					search_direction = id_bit;
				}
				else
				{
// If this discrepancy if before the Last Discrepancy on a previous next then pick the same as last time
					if (id_bit_number < param->last_discrepancy)
						search_direction = ((param->rom_no[rom_byte_number] & rom_byte_mask) > 0);
					else
// If equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == param->last_discrepancy);

// If 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

// Check for Last discrepancy in family
						if (last_zero < 9)
							param->last_family_discrepancy = last_zero;
					}
				}

// Set or clear the bit in the ROM byte rom_byte_number with mask rom_byte_mask
				if (search_direction == 1)
					param->rom_no[rom_byte_number] |= rom_byte_mask;
				else
					param->rom_no[rom_byte_number] &= ~rom_byte_mask;

// Serial number search direction write bit
				if (!owuart_touch_bit(search_direction, 0))
					return(0);

// Increment the byte counter id_bit_number and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

// If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
// Accumulate the CRC
					crc8 = owcrc8_table[crc8 ^ param->rom_no[rom_byte_number]];
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
// Loop until through all ROM bytes 0-7
		} while(rom_byte_number < 8);

// If the search was successful then
		if (!((id_bit_number < 65) || (crc8 != 0)))
		{
// Search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			param->last_discrepancy = last_zero;

// Check for last device
			if (param->last_discrepancy == 0)
				param->last_device_flag = 1;

			search_result = 1;
		}
	}

// If no device found then reset counters so next 'search' will be like a first
	if (!search_result || !param->rom_no[0])
	{
		search_result = 0;
		param->last_device_flag = 0;
		param->last_discrepancy = 0;
		param->last_family_discrepancy = 0;
	}

	return(search_result);
}

void owuart_test_search()
{
	struct owuart_search_param param;

	param.last_device_flag = 0;
	param.last_discrepancy = 0;
	param.last_family_discrepancy = 0;
	while(owuart_search(&param))
	{
		printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", param.rom_no[0], param.rom_no[1],
			param.rom_no[2], param.rom_no[3], param.rom_no[4], param.rom_no[5],
			param.rom_no[6], param.rom_no[7]);
	}
}
