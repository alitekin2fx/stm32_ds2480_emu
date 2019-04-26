/*
 * ds2480b.c
 *
 *  Created on: Apr 24, 2019
 *      Author: ali-teke
 */

#include "main.h"
#include "stm32f0xx_hal.h"
#include "usbd_cdc_if.h"
#include "ow_uart.h"
#include "ds2480b.h"

// Mode Commands
#define MODE_DATA					0xe1
#define MODE_COMMAND				0xe3
#define MODE_STOP_PULSE				0xf1

/* Command or config bit */
#define CMD_COMMAND					0x81
#define CMD_CONFIG					0x01
#define CMD_MASK					0x81

/* Command function bits */
#define CMDFUNC_SINGLEBIT			0x00
#define CMDFUNC_SEARCHCTRL			0x20
#define CMDFUNC_RESET				0x40
#define CMDFUNC_PULSE				0x60
#define CMDFUNC_MASK				0x60

/* Search accelerator on / off */
#define SEARCHCTRL_ON				0x10
#define SEARCHCTRL_OFF				0x00
#define SEARCHCTRL_MASK				0x10

/* Single bit write */
#define SINGLEBITWRITE_ONE			0x10
#define SINGLEBITWRITE_ZERO			0x00
#define SINGLEBITWRITE_MASK			0x10

/* Single bit strong pull up */
#define SINGLEBITSPUP_ON			0x02
#define SINGLEBITSPUP_OFF			0x00
#define SINGLEBITSPUP_MASK			0x02

/* Pulse mode */
#define PULSEMODE_5VPULLUP			0x00
#define PULSEMODE_12VPULSE			0x10
#define PULSEMODE_MASK				0x10

/* Pulse arm / disarm */
#define PULSEARM_ARM				0x02
#define PULSEARM_DISARM				0x00
#define PULSEARM_MASK				0x02

/* One Wire speed bits */
#define SPEED_REGULAR				0x00
#define SPEED_FLEXIBLE				0x04
#define SPEED_OVERDRIVE				0x08
#define SPEED_PULSE					0x0c
#define SPEED_MASK					0x0c

/* Config write parameter */
#define WPARMCODE_SLEW              0x10
#define WPARMCODE_PRGPULSE          0x20
#define WPARMCODE_PULLUP            0x30
#define WPARMCODE_WRITE1LOW         0x40
#define WPARMCODE_SAMPLEOFFSET      0x50
#define WPARMCODE_ACTIVEPULLUPTIME  0x60
#define WPARMCODE_BAUDRATE          0x70
#define WPARMCODE_MASK              0x70

/* Config read parameter */
#define RPARMCODE_SLEW              0x02
#define RPARMCODE_PRGPULSE          0x04
#define RPARMCODE_PULLUP            0x06
#define RPARMCODE_WRITE1LOW         0x08
#define RPARMCODE_SAMPLEOFFSET      0x0a
#define RPARMCODE_ACTIVEPULLUPTIME  0x0c
#define RPARMCODE_BAUDRATE          0x0e
#define RPARMCODE_MASK              0x0e

/* Single bit response */
#define SINGLEBITRESP_ONE			0x03
#define SINGLEBITRESP_ZERO			0x00

/* Reset response */
#define RESETRESP_1WIRESHORT		0x00
#define RESETRESP_PRESENCE			0x01
#define RESETRESP_ALARMPRESENCE		0x02
#define RESETRESP_NOPRESENCE		0x03

static void ds2480b_handle_command(struct ds2480b *ds2480b, uint8_t data);
static void ds2480b_handle_check(struct ds2480b *ds2480b, uint8_t data);
static void ds2480b_handle_data_internal(struct ds2480b *ds2480b, uint8_t data);
static void ds2480b_cmdfunc_singlebit(struct ds2480b *ds2480b, uint8_t write_value,
	uint8_t speed, uint8_t spup);
static void ds2480b_cmdfunc_searchctrl(struct ds2480b *ds2480b, uint8_t on_off, uint8_t speed);
static void ds2480b_cmdfunc_reset(struct ds2480b *ds2480b, uint8_t speed);
static void ds2480b_cmdfunc_pulse(struct ds2480b *ds2480b, uint8_t mode, uint8_t arm_disarm);
static void ds2480b_handle_write_config(struct ds2480b *ds2480b, uint8_t code, int parameter_value_code);
static void ds2480b_handle_read_config(struct ds2480b *ds2480b, uint8_t code);
static int ds2480b_transmit_byte(struct ds2480b *ds2480b, uint8_t value);
static int ds2480b_transmit_data(struct ds2480b *ds2480b, const uint8_t *data, int data_length);
static void ds2480b_search(struct ds2480b *ds2480b);
static int get_bit(const uint8_t *buffer, int address);
static void set_bit(uint8_t *buffer, int address, int value);

void ds2480b_reset(struct ds2480b *ds2480b)
{
	ds2480b->acc_on = 0;
	ds2480b->mode = DS2480B_MODE_COMMAND;
}

void ds2480b_handle_data(struct ds2480b *ds2480b, const uint8_t *data, int data_length)
{
	for(int i = 0; i < data_length; i++)
	{
		if (ds2480b->mode == DS2480B_MODE_COMMAND)
			ds2480b_handle_command(ds2480b, data[i]);
		else if (ds2480b->mode == DS2480B_MODE_CHECK)
			ds2480b_handle_check(ds2480b, data[i]);
		else if (ds2480b->mode == DS2480B_MODE_DATA)
			ds2480b_handle_data_internal(ds2480b, data[i]);
	}
}

void ds2480b_handle_command(struct ds2480b *ds2480b, uint8_t data)
{
	if (data == MODE_DATA)
	{
		ds2480b->mode = DS2480B_MODE_DATA;
	}
	else if (data != MODE_STOP_PULSE && data != MODE_COMMAND)
	{
		switch(data & CMD_MASK)
		{
		case CMD_COMMAND:
			switch(data & CMDFUNC_MASK)
			{
			case CMDFUNC_SINGLEBIT:
				ds2480b_cmdfunc_singlebit(ds2480b, data & SINGLEBITWRITE_MASK,
					data & SPEED_MASK, data & SINGLEBITSPUP_MASK);
				break;
			case CMDFUNC_SEARCHCTRL:
				ds2480b_cmdfunc_searchctrl(ds2480b, data & SEARCHCTRL_MASK,
					data & SPEED_MASK);
				break;
			case CMDFUNC_RESET:
				ds2480b_cmdfunc_reset(ds2480b, data & SPEED_MASK);
				break;
			case CMDFUNC_PULSE:
				ds2480b_cmdfunc_pulse(ds2480b, data & PULSEMODE_MASK, data & PULSEARM_MASK);
				break;
			}
			break;
		case CMD_CONFIG:
			if (data & WPARMCODE_MASK)
				ds2480b_handle_write_config(ds2480b, data & WPARMCODE_MASK, (data >> 1) & 7);
			else
				ds2480b_handle_read_config(ds2480b, data & RPARMCODE_MASK);
		}
	}
}

void ds2480b_handle_check(struct ds2480b *ds2480b, uint8_t data)
{
	if (data == ds2480b->check_value)
	{
/* If both bytes are the same, the byte is sent once to the 1-Wire bus and the
 * device returns to the Data Mode */
		ds2480b_transmit_byte(ds2480b, ow_uart_touch_byte(data));
		ds2480b->mode = DS2480B_MODE_DATA;
	}
	else
	{
/* If the second byte is different from the reserved code, it will be executed as command
 * and the device finally enters the Command Mode */
		ds2480b_handle_command(ds2480b, data);
		ds2480b->mode = DS2480B_MODE_COMMAND;
	}
}

void ds2480b_handle_data_internal(struct ds2480b *ds2480b, uint8_t data)
{
	if (data == MODE_COMMAND)
	{
		ds2480b->check_value = data;
		ds2480b->mode = DS2480B_MODE_CHECK;
	}
	else
	{
		if (ds2480b->acc_on)
		{
			if (ds2480b->search_data_count < 16)
			{
				ds2480b->search_data[ds2480b->search_data_count++] = data;
				if (ds2480b->search_data_count == 16)
				{
					ds2480b_search(ds2480b);
					ds2480b->search_data_count = 0;
				}
			}
		}
		else
		{
			ds2480b_transmit_byte(ds2480b, ow_uart_touch_byte(data));
		}
	}
}

void ds2480b_cmdfunc_singlebit(struct ds2480b *ds2480b, uint8_t write_value,
	uint8_t speed, uint8_t spup)
{
	int bit_value = ow_uart_touch_bit((write_value == SINGLEBITWRITE_ONE) ? 1 : 0);
	ds2480b_transmit_byte(ds2480b, 0x80 | write_value | speed | (bit_value ?
			SINGLEBITRESP_ONE : SINGLEBITRESP_ZERO));

	if (spup == SINGLEBITSPUP_ON)
		ds2480b_transmit_byte(ds2480b, bit_value ? 0xef : 0xec);
}

void ds2480b_cmdfunc_searchctrl(struct ds2480b *ds2480b, uint8_t on_off, uint8_t speed)
{
	ds2480b->acc_on = on_off;
	ds2480b->search_data_count = 0;
}

void ds2480b_cmdfunc_reset(struct ds2480b *ds2480b, uint8_t speed)
{
	uint8_t resp_data;

	HAL_GPIO_WritePin(OW_SYNC_GPIO_Port, OW_SYNC_Pin, GPIO_PIN_SET);
	int res = ow_uart_touch_reset();
	HAL_GPIO_WritePin(OW_SYNC_GPIO_Port, OW_SYNC_Pin, GPIO_PIN_RESET);

	if (res == 2)
		resp_data = RESETRESP_PRESENCE;
	else if (res == 0)
		resp_data = RESETRESP_1WIRESHORT;
	else
		resp_data = RESETRESP_NOPRESENCE;
	ds2480b_transmit_byte(ds2480b, resp_data | 0xcc);
}

void ds2480b_cmdfunc_pulse(struct ds2480b *ds2480b, uint8_t mode, uint8_t arm_disarm)
{
	ds2480b_transmit_byte(ds2480b, mode | arm_disarm | 0xe0);
}

void ds2480b_handle_write_config(struct ds2480b *ds2480b, uint8_t code, int parameter_value_code)
{
	switch(code)
	{
	case WPARMCODE_SLEW:
		ds2480b->slew = parameter_value_code;
		break;
	case WPARMCODE_PRGPULSE:
		ds2480b->prg_pulse = parameter_value_code;
		break;
	case WPARMCODE_PULLUP:
		ds2480b->pullup = parameter_value_code;
		break;
	case WPARMCODE_WRITE1LOW:
		ds2480b->write_1_low = parameter_value_code;
		break;
	case WPARMCODE_SAMPLEOFFSET:
		ds2480b->sample_offset = parameter_value_code;
		break;
	case WPARMCODE_ACTIVEPULLUPTIME:
		ds2480b->active_pullup_time = parameter_value_code;
		break;
	case WPARMCODE_BAUDRATE:
		ds2480b->baudrate = parameter_value_code;
		break;
	}
	ds2480b_transmit_byte(ds2480b, code | (parameter_value_code << 1));
}

void ds2480b_handle_read_config(struct ds2480b *ds2480b, uint8_t code)
{
	int parameter_value_code;

	switch(code)
	{
	case RPARMCODE_SLEW:
		parameter_value_code = ds2480b->slew;
		break;
	case RPARMCODE_PRGPULSE:
		parameter_value_code = ds2480b->prg_pulse;
		break;
	case RPARMCODE_PULLUP:
		parameter_value_code = ds2480b->pullup;
		break;
	case RPARMCODE_WRITE1LOW:
		parameter_value_code = ds2480b->write_1_low;
		break;
	case RPARMCODE_SAMPLEOFFSET:
		parameter_value_code = ds2480b->sample_offset;
		break;
	case RPARMCODE_ACTIVEPULLUPTIME:
		parameter_value_code = ds2480b->active_pullup_time;
		break;
	case RPARMCODE_BAUDRATE:
		parameter_value_code = ds2480b->baudrate;
		break;
	default:
		parameter_value_code = 0;
		break;
	}
	ds2480b_transmit_byte(ds2480b, parameter_value_code << 1);
}

int ds2480b_transmit_byte(struct ds2480b *ds2480b, uint8_t value)
{
	return(ds2480b_transmit_data(ds2480b, &value, sizeof(value)));
}

int ds2480b_transmit_data(struct ds2480b *ds2480b, const uint8_t *data, int data_length)
{
	uint32_t tick_count;

	tick_count = HAL_GetTick();
	while(CDC_Transmit_FS((uint8_t*)data, data_length) != USBD_OK)
	{
		if (HAL_GetTick() - tick_count > 500)
			return(0);
	}
	return(1);
}

void ds2480b_search(struct ds2480b *ds2480b)
{
	uint8_t search_result[16];

	for(int i = 0; i < 64; i++)
	{
		int id_bit, cmp_id_bit;
		int discrepancy, chosen_path;

		/* For each bit position n (values from 0 to 63) the DS2480B will generate three
		 * time slots on the 1-Wire bus. These are referenced as:
		 * b0: for the first time slot (Read Data)
		 * b1: for the second time slot (Read Data) and
		 * b2: for the third time slot (Write Data) */

		/* Read a bit and its complement (b0 and b1) */
		id_bit = ow_uart_touch_bit(1);
		cmp_id_bit = ow_uart_touch_bit(1);

		/* Check if there is no response */
		if (id_bit == 1 && cmp_id_bit == 1)
		{
			chosen_path = 1;
			discrepancy = 0;
		}
		else if (id_bit != cmp_id_bit)
		{
			chosen_path = id_bit;
			discrepancy = 0;
		}
		else
		{
			chosen_path = get_bit(ds2480b->search_data, (i * 2) + 1);
			discrepancy = 1;
		}

		/* The type of time slot b2 (write 1 or write 0) is determined by the DS2480B as follows:
		 * b2 = rn if conflict (as chosen by the host)
		 * b2 = b0 if no conflict (there is no alternative)
		 * b2 = 1 if error (there is no response) */
		ow_uart_touch_bit(chosen_path);

		set_bit(search_result, (i * 2) + 0, discrepancy);
		set_bit(search_result, (i * 2) + 1, chosen_path);
	}

	ds2480b_transmit_data(ds2480b, search_result, sizeof(search_result));
}

int get_bit(const uint8_t *buffer, int address)
{
	int byte_number, bit_number;

	byte_number = (address / 8);
	bit_number = address - (byte_number * 8);

	return((buffer[byte_number] & (1 << bit_number)) != 0);
}

void set_bit(uint8_t *buffer, int address, int value)
{
	int byte_number, bit_number;

	byte_number = (address / 8);
	bit_number = address - (byte_number * 8);

	if (value)
		buffer[byte_number] |= (1 << bit_number);
	else
		buffer[byte_number] &= ~(1 << bit_number);
}
