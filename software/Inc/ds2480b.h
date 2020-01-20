#ifndef __DS2480B_H__
#define __DS2480B_H__

enum DS2480B_MODE
{
	DS2480B_MODE_COMMAND,
	DS2480B_MODE_DATA,
	DS2480B_MODE_CHECK
};

struct ds2480b
{
	int slew;
	int acc_on;
	int pullup;
	int baudrate;
	int prg_pulse;
	int write_1_low;
	int sample_offset;
	uint8_t check_value;
	int active_pullup_time;
	enum DS2480B_MODE mode;
};

void ds2480b_reset(struct ds2480b *ds2480b);
void ds2480b_handle_data(struct ds2480b *ds2480b, const uint8_t *data, int data_length);

#endif /* __DS2480B_H__ */
