#ifndef __OWUART_H__
#define __OWUART_H__

struct owuart_search_param
{
	uint8_t rom_no[8];
	int last_device_flag;
	int last_discrepancy;
	int last_family_discrepancy;
};

void owuart_init();
int owuart_touch_reset();
int owuart_touch_bit(int tx_bit, int *rx_bit);
int owuart_touch_byte(uint8_t tx_byte, uint8_t *rx_byte);
int owuart_search(struct owuart_search_param *param);

void owuart_test_search();

#endif /* __OWUART_H__ */
