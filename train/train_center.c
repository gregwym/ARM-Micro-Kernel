#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

#define DELAY_REVERSE 200
#define DELAY_SWITCH 40

/* Helpers */
inline int switchIdToIndex(int id) {
	return id % SWITCH_INDEX_MOD - 1;
}

/* Workers */
void switchChanger(int switch_id, int direction) {
	char cmd[2];
	cmd[0] = direction;
	cmd[1] = switch_id;
	Puts(COM1, cmd, 2);

	// Delay 400ms then turn off the solenoid
	Delay(DELAY_SWITCH);

	Putc(COM1, SWITCH_OFF);
}

void trainReverser(int train_id, int new_speed) {
	char cmd[2];
	cmd[0] = 0;
	cmd[1] = train_id;
	Puts(COM1, cmd, 2);

	// Delay 2s then turn off the solenoid
	Delay(DELAY_REVERSE);

	// Reverse
	cmd[0] = TRAIN_REVERSE;
	Puts(COM1, cmd, 2);

	// Re-accelerate
	cmd[0] = new_speed;
	Puts(COM1, cmd, 2);
}

/* Infomation Handlers */
inline void handleSensorUpdate(char *new_data, char *saved_data, char *buf) {
	int i, j;
	char old_byte, new_byte, old_bit, new_bit, decoder_id;

	char *buf_cursor = buf;
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		// Fetch byte and save
		old_byte = saved_data[i];
		new_byte = new_data[i];
		saved_data[i] = new_byte;

		// If the byte changed
		if(old_byte != new_byte) {
			decoder_id = 'A' + (i / 2);

			for(j = 0; j < SENSOR_BYTE_SIZE; j++) {
				old_bit = old_byte & SENSOR_BIT_MASK;
				new_bit = new_byte & SENSOR_BIT_MASK;

				// If the bit changed
				if(old_bit != new_bit) {
					int sensor_id = (SENSOR_BYTE_SIZE * (i % 2)) + (SENSOR_BYTE_SIZE - j);
					buf_cursor += sprintf(buf_cursor, "%c%d -> %d, ", decoder_id, sensor_id, new_bit);
				}
				old_byte = old_byte >> 1;
				new_byte = new_byte >> 1;
			}
		}
	}
	if(buf != buf_cursor) {
		sprintf(buf_cursor, "\n");
		Puts(COM2, buf, 0);
	}
}

inline void handleSwitchCommand(int id, int value, char *switch_table, char *buf) {
	sprintf(buf, "sw: %d -> %c\n", id, value);

	value = value == 'C' ? SWITCH_CUR : SWITCH_STR;
	CreateWithArgs(1, switchChanger, id, value, 0, 0);

	switch_table[switchIdToIndex(id)] = value;
	Puts(COM2, buf, 0);
}

/* Train Center */
void trainCenter() {
	/* TrainGlobal */
	TrainGlobal *train_global;
	int i, tid, cmd_tid, sensor_tid, result;
	result = Receive(&tid, (char *)(&train_global), sizeof(TrainGlobal *));
	assert(result >= 0, "Train central failed to receive TrainGlobal");

	/* SensorData */
	char sensor_data[SENSOR_BYTES_TOTAL];
	char sensor_decoder_ids[SENSOR_DECODER_TOTAL];
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		sensor_data[i] = 0;
	}
	for(i = 0; i < SENSOR_DECODER_TOTAL; i++) {
		sensor_decoder_ids[i] = 'A' + i;
	}

	/* SwitchData */
	char switch_table[SWITCH_TOTAL];
	for(i = 0; i < SWITCH_TOTAL; i++) {
		switch_table[i] = '?';
	}
	train_global->switch_table = switch_table;

	cmd_tid = Create(7, trainCmdNotifier);
	sensor_tid = Create(7, trainSensorNotifier);

	TrainMsg msg;
	char str_buf[1024];

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainCentral receive failed");

		switch (msg.type) {
			case SENSOR_DATA:
				Reply(tid, NULL, 0);
				handleSensorUpdate(msg.sensor_msg.sensor_data, sensor_data, str_buf);
				break;
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				// TODO: Send command to the train
				str_buf[0] = msg.cmd_msg.value;
				str_buf[1] = msg.cmd_msg.id;
				Puts(COM1, str_buf, 2);
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				// TODO: Send command to the train
				CreateWithArgs(1, trainReverser, msg.cmd_msg.id, 10, 0, 0);
				break;
			case CMD_SWITCH:
				Reply(tid, NULL, 0);
				handleSwitchCommand(msg.cmd_msg.id, msg.cmd_msg.value, switch_table, str_buf);
				break;
			case CMD_QUIT:
				Halt();
				break;
			default:
				sprintf(str_buf, "Got unknown msg, type: %d\n", msg.type);
				Puts(COM2, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}
