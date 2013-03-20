#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

#define SWITCH_DELAY		40
#define SWITCH_INIT_DELAY	500

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
void switchChanger(int switch_id, int direction, int com1_tid, int com2_tid) {
	char state = (direction == 33 ? 'S' : 'C');
	if (switch_id <= 18) {
		iprintf(com2_tid, 20, "\e[s\e[%d;%dH%c\e[u", switch_id/9 + 6, ((switch_id - 1)%9)*6 + 6, state);
	} else if (switch_id >= 153 && switch_id <= 156) {
		iprintf(com2_tid, 20, "\e[s\e[%d;%dH%c\e[u", 8, ((switch_id - 153)%9)*6 + 6, state);
	}
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
					// int sensor_id = (SENSOR_BYTE_SIZE * (i % 2)) + (SENSOR_BYTE_SIZE - j);
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

	// sprintf(buf, "sw #%d[%d] %d -> %d\n", id, switchIdToIndex(id),
	        // switch_table[switchIdToIndex(id)], value);

	CreateWithArgs(2, switchChanger, id, value, train_global->com1_tid, train_global->com2_tid);

	switch_table[switchIdToIndex(id)] = value;
	// Puts(train_global->com2_tid, buf, 0);
}

void reserveTrack(TrainGlobal *train_global, int train_id, int landmark_id, int distance) {
	int *track_reservation = train_global->track_reservation;
	track_node *track_nodes = train_global->track_nodes;
	track_node *current = &(track_nodes[landmark_id]);

	// Reserve current landmark
	track_reservation[current->index] = train_id;
	track_reservation[current->reverse->index] = train_id;

	// Reserve along the way until run out of distance or reach a Branch/Exit
	while(distance >= 0 && current->type != NODE_BRANCH && current->type != NODE_EXIT) {
		// Deduct current the distance to the next landmark
		distance -= current->edge[DIR_AHEAD].dist;

		// Set current to the next
		current = current->edge[DIR_AHEAD].dest;

		// Reserve both direction node
		track_reservation[current->index] = train_id;
		track_reservation[current->reverse->index] = train_id;
	}

	if(distance < 0 || current->type == NODE_EXIT) return;

	// If reach a branch
	reserveTrack(train_global, train_id, current->edge[DIR_STRAIGHT].dest->index, distance - current->edge[DIR_STRAIGHT].dist);
	reserveTrack(train_global, train_id, current->edge[DIR_CURVED].dest->index, distance - current->edge[DIR_CURVED].dist);
}

int trackAvailability(TrainGlobal *train_global, int train_id, int landmark_id, int distance) {
	int *track_reservation = train_global->track_reservation;
	track_node *track_nodes = train_global->track_nodes;
	track_node *current = &(track_nodes[landmark_id]);

	DEBUG(DB_RESERVE, "%s\t%d\n", current->name, distance);

	// Check current landmark
	if(track_reservation[current->index] != -1 &&
	   track_reservation[current->index] != train_id) {
	   	DEBUG(DB_RESERVE, "%s\tF\n", current->name);
		return FALSE;
	}

	// Check along the way until run out of distance or reach a Branch/Exit
	while(distance >= 0 && current->type != NODE_BRANCH && current->type != NODE_EXIT) {
		// Deduct current the distance to the next landmark
		distance -= current->edge[DIR_AHEAD].dist;

		// Set current to the next
		current = current->edge[DIR_AHEAD].dest;

		DEBUG(DB_RESERVE, "%s\t%d\n", current->name, distance);
		// Check both direction node
		if(track_reservation[current->index] != -1 &&
		   track_reservation[current->index] != train_id) {
		   	bwprintf(COM2, "%s\tF\n", current->name);
			return FALSE;
		}
	}

	if(distance < 0 || current->type == NODE_EXIT) {
		DEBUG(DB_RESERVE, "%s\tT\n", current->name);
		return TRUE;
	}

	// If reach a branch
	return trackAvailability(train_global, train_id, current->edge[DIR_STRAIGHT].dest->index, distance - current->edge[DIR_STRAIGHT].dist) &&
	trackAvailability(train_global, train_id, current->edge[DIR_CURVED].dest->index, distance - current->edge[DIR_CURVED].dist);
}

inline TrainMsgType handleTrackReserve(TrainGlobal *train_global, int train_id, int landmark_id, int distance) {
	TrainData *train_data = train_global->train_data;
	TrainData *train = NULL;
	int i;

	for(i = 0; i < TRAIN_MAX; i++) {
		if(train_data[i].id == train_id) {
			train = &(train_data[i]);
			break;
		}
	}
	assert(train != NULL, "handleTrackReserve: Unable to find train_id");

	if(!trackAvailability(train_global, train_id, landmark_id, distance)) {
		return TRACK_RESERVE_FAIL;
	}

	// Clear previous reservation
	reserveTrack(train_global, -1, train->reservation_record.landmark_id, train->reservation_record.distance);

	// Reserve for this time
	reserveTrack(train_global, train_id, landmark_id, distance);

	// Save reservation record
	train->reservation_record.landmark_id = landmark_id;
	train->reservation_record.distance = distance;

	return TRACK_RESERVE_SUCCEED;
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
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				// assert(msg.cmd_msg.id < TRAIN_NUM_MAX, "Exceed max train number");
				if (msg.cmd_msg.id < TRAIN_NUM_MAX) {
					tid = train_tid[msg.cmd_msg.id];
					if(tid >= 0) {
						CreateWithArgs(2, cmdPostman, train_tid[msg.cmd_msg.id], msg.type, msg.cmd_msg.value, 0);
					}
				}
				break;
			case CMD_SWITCH:
				Reply(tid, NULL, 0);
				handleSwitchCommand(&(msg.cmd_msg), train_global, str_buf);
				break;
			case CMD_QUIT:
				Halt();
				break;
			case TRACK_RESERVE:
				msg.type = handleTrackReserve(train_global, msg.reservation_msg.train_id, msg.reservation_msg.landmark_id, msg.reservation_msg.distance);
				Reply(tid, (char *)(&(msg.type)), sizeof(TrainMsgType));
				break;
			default:
				sprintf(str_buf, "Center got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}
