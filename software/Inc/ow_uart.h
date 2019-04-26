#ifndef __OW_UART_H__
#define __OW_UART_H__

struct ow_uart_search_param
{
	uint8_t rom_no[8];
	int lastDeviceFlag;
	int lastDiscrepancy;
	int lastFamilyDiscrepancy;
};

int ow_uart_touch_reset();
int ow_uart_touch_bit(int bit_value);
uint8_t ow_uart_touch_byte(uint8_t byte_value);
uint8_t ow_uart_search(struct ow_uart_search_param *param);

void ow_uart_test_search();

#endif /* __OW_UART_H__ */
