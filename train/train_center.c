#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

#define SWITCH_DELAY		40
#define SWITCH_INIT_DELAY	500
#define TRAIN_NUM_MAX		50
#define TRAIN_INIT_DELAY	200

/* Helpers */
inline int switchIdToIndex(int id) {
	return id % SWITCH_INDEX_MOD - 1;
}

inline int sensorIdToLandmark(int byte_id, int bit_id) {
	return (byte_id + 1) * SENSOR_BYTE_SIZE - bit_id - 1;
}

inline void changeSwitch(int switch_id, int direction, int com1_tid) {
	char cmd[2];
	cmd[0] = direction;
	cmd[1] = switch_id;
	Puts(com1_tid, cmd, 2);
}

/* Workers */
void switchChanger(int switch_id, int direction, int com1_tid) {
	changeSwitch(switch_id, direction, com1_tid);

	// Delay 400ms then turn off the solenoid
	Delay(SWITCH_DELAY);

	Putc(com1_tid, SWITCH_OFF);
}

void cmdPostman(int tid, TrainMsgType type, int value) {
	CmdMsg msg;
	msg.type = type;
	msg.value = value;
	Send(tid, (char *)(&msg), sizeof(CmdMsg), NULL, 0);
}

void locationPostman(int tid, int landmark_id, int value) {
	LocationMsg msg;
	msg.type = LOCATION_CHANGE;
	msg.id = landmark_id;
	msg.value = value;
	Send(tid, (char *)(&msg), sizeof(LocationMsg), NULL, 0);
}

/* Infomation Handlers */
inline void handleSensorUpdate(char *new_data, char *saved_data, char *buf, TrainGlobal *train_global) {
	int i, j, k;
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
				if(old_bit != new_bit && new_bit) {
					int sensor_id = (SENSOR_BYTE_SIZE * (i % 2)) + (SENSOR_BYTE_SIZE - j);
					// buf_cursor += sprintf(buf_cursor, "%c%d -> %d, ", decoder_id, sensor_id, new_bit);
					// Deliver location change to drivers
					for(k = 0; k < TRAIN_MAX; k++) {
						CreateWithArgs(2, locationPostman, train_global->train_data[k].tid,
						               sensorIdToLandmark(i, j), new_bit, 0);
					}
				}
				old_byte = old_byte >> 1;
				new_byte = new_byte >> 1;
			}
		}
	}
	if(buf != buf_cursor) {
		sprintf(buf_cursor, "\n");
		Puts(train_global->com2_tid, buf, 0);
	}
}

inline void handleSwitchCommand(CmdMsg *msg, TrainGlobal *train_global, char *buf) {
	int id = msg->id;
	int value = msg->value;
	char *switch_table = train_global->switch_table;

	sprintf(buf, "sw #%d[%d] %d -> %d\n", id, switchIdToIndex(id),
	        switch_table[switchIdToIndex(id)], value);

	CreateWithArgs(2, switchChanger, id, value, train_global->com1_tid, 0);

	switch_table[switchIdToIndex(id)] = value;
	Puts(train_global->com2_tid, buf, 0);
}

/* Train Center */
void trainCenter(TrainGlobal *train_global) {

	int i, tid, cmd_tid, sensor_tid, result, com1_tid, com2_tid;
	TrainMsg msg;
	char str_buf[1024];

	com1_tid = train_global->com1_tid;
	com2_tid = train_global->com2_tid;

	Putc(com1_tid, SYSTEM_START);

	/* SensorData */
	char sensor_data[SENSOR_BYTES_TOTAL];
	memset(sensor_data, 0, SENSOR_BYTES_TOTAL);
	char sensor_decoder_ids[SENSOR_DECODER_TOTAL];
	for(i = 0; i < SENSOR_DECODER_TOTAL; i++) {
		sensor_decoder_ids[i] = 'A' + i;
	}

	/* Initialize Train Drivers */
	TrainData *train_data = train_global->train_data;
	int train_tid[TRAIN_NUM_MAX];
	memset(train_tid, -1, sizeof(int) * TRAIN_NUM_MAX);
	for(i = 0; i < TRAIN_MAX; i++) {
		assert(train_data[i].id < TRAIN_NUM_MAX, "Exceed max train number");
		train_data[i].tid = CreateWithArgs(9, trainDriver, (int)train_global, (int)&(train_data[i]), 0, 0);
		train_tid[train_data[i].id] = train_data[i].tid;
	}
	Delay(TRAIN_INIT_DELAY);

	/* Initialize switches */
	for(i = 0; i < SWITCH_TOTAL; i++) {
		changeSwitch(i < SWITCH_NAMING_MAX ? i + 1 :
		             i - SWITCH_NAMING_MAX + SWITCH_NAMING_MID_BASE, SWITCH_CUR, com1_tid);
	}
	Delay(SWITCH_INIT_DELAY);
	Putc(com1_tid, SWITCH_OFF);

	cmd_tid = Create(7, trainCmdNotifier);
	sensor_tid = Create(7, trainSensorNotifier);

	Puts(com2_tid, "\e[2JInitialized\n", 0);

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainCentral receive failed");

		switch (msg.type) {
			case SENSOR_DATA:
				Reply(tid, NULL, 0);
				handleSensorUpdate(msg.sensor_msg.sensor_data, sensor_data, str_buf, train_global);
				break;
			case CMD_SPEED:
			case CMD_GOTO:
				Reply(tid, NULL, 0);
				assert(msg.cmd_msg.id < TRAIN_NUM_MAX, "Exceed max train number");
				tid = train_tid[msg.cmd_msg.id];
				if(tid >= 0) {
					CreateWithArgs(2, cmdPostman, train_tid[msg.cmd_msg.id], msg.type, msg.cmd_msg.value, 0);
				}
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				assert(msg.cmd_msg.id < TRAIN_NUM_MAX, "Exceed max train number");
				tid = train_tid[msg.cmd_msg.id];
				if(tid >= 0) {
					CreateWithArgs(2, cmdPostman, train_tid[msg.cmd_msg.id], CMD_REVERSE, 0, 0);
				}
				break;
			case CMD_SWITCH:
				Reply(tid, NULL, 0);
				handleSwitchCommand(&(msg.cmd_msg), train_global, str_buf);
				break;
			case CMD_QUIT:
				Halt();
				break;
			default:
				sprintf(str_buf, "Center got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}
