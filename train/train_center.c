#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <kern/md_const.h>
#include <ts7200.h>

#define SWITCH_DELAY		40
#define SWITCH_INIT_DELAY	500

#define TRAIN_INIT_DELAY	200
#define RECOVERY_TIME_THRESHOLD	4000

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
void switchChanger(TrainGlobal *train_global, int switch_id, int direction) {
	char state = (direction == 33 ? 'S' : 'C');

	int index = switch_id > SWITCH_NAMING_MAX ? switch_id - SWITCH_NAMING_MID_BASE + SWITCH_NAMING_MAX : switch_id - SWITCH_NAMING_BASE;
	int ui_row = index % HEIGHT_SWITCH_TABLE + ROW_SWITCH_TABLE;
	int ui_column = (index / HEIGHT_SWITCH_TABLE) * COLUMN_WIDTH * 2 + COLUMN_VALUES + COLUMN_WIDTH;
	train_global->switch_table[index] = direction;

	uiprintf(train_global->com2_tid, ui_row, ui_column, "%c", state);
	changeSwitch(switch_id, direction, train_global->com1_tid);

	// Delay 400ms then turn off the solenoid
	Delay(SWITCH_DELAY);

	Putc(train_global->com1_tid, SWITCH_OFF);
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

int isContinuousSensor(track_node *src, track_node *dest, int count) {
	count--;
	switch(src->type) {
		case NODE_SENSOR:
			if (count < 0) {
				return src == dest;
			}
		case NODE_ENTER:
		case NODE_MERGE:
			return isContinuousSensor(src->edge[DIR_AHEAD].dest, dest, count);
		case NODE_BRANCH:
			return isContinuousSensor(src->edge[DIR_STRAIGHT].dest, dest, count) ||
				   isContinuousSensor(src->edge[DIR_CURVED].dest, dest, count);
		default:
			break;
	}

	return FALSE;
}

/* Infomation Handlers */
inline void handleSensorUpdate(TrainGlobal *train_global, CenterData *center_data, char *new_data) {
	int i, j, landmark_id;
	char old_byte, new_byte, old_bit, new_bit;
	TrainData *recovery_train_data, *train_data;
	char *saved_data = center_data->sensor_data;

	char buf[128];
	char *buf_cursor = buf;
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		// Fetch byte and save
		old_byte = saved_data[i];
		new_byte = new_data[i];
		saved_data[i] = new_byte;

		// If the byte changed
		if(old_byte != new_byte) {

			for(j = 0; j < SENSOR_BYTE_SIZE; j++) {
				old_bit = old_byte & SENSOR_BIT_MASK;
				new_bit = new_byte & SENSOR_BIT_MASK;

				// If the bit changed
				if(old_bit != new_bit && (!new_bit)) {
					landmark_id = sensorIdToLandmark(i, j);
					recovery_train_data = train_global->recovery_reservation[landmark_id];
					train_data = train_global->track_reservation[landmark_id];
					if(recovery_train_data != NULL) {
						CreateWithArgs(2, locationPostman, recovery_train_data->tid, landmark_id, new_bit, 0);
						IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 3, recovery_train_data->index * WIDTH_DEBUG, "#%s => %d  ", train_global->track_nodes[landmark_id].name, recovery_train_data->id);
					} else if(train_data != NULL) {
						CreateWithArgs(2, locationPostman, train_data->tid, landmark_id, new_bit, 0);
						IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 3, train_data->index * WIDTH_DEBUG, "#%s => %d  ", train_global->track_nodes[landmark_id].name, train_data->id);
					} else {
						track_node *current_sensor = &(train_global->track_nodes[landmark_id]);
						IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 4, (center_data->lost_count % 10) * COLUMN_WIDTH, "#%s  ", current_sensor->name);
						if(center_data->last_lost_sensor != NULL && isContinuousSensor(center_data->last_lost_sensor, current_sensor, 1)) {
							// assert(0, "Train is lost");
							IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 5, 0, "Train lost at %s", current_sensor->name);
						}
						center_data->last_lost_sensor = current_sensor;
						center_data->last_lost_timestamp = getTimerValue(TIMER3_BASE);
						center_data->lost_count++;
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

inline void handleSwitchCommand(CmdMsg *msg, TrainGlobal *train_global) {
	int id = msg->id;
	int value = msg->value;

	CreateWithArgs(2, switchChanger, (int)train_global, id, value, 0);
}

typedef struct traverse_config {
	int landmark_id;
	int distance;
	int num_sensor;
	int take_branch;
	track_node *(*normal)(TrainData *train_data, track_node *current, TrainData **reservation);
	track_node *(*recovery)(TrainData *train_data, track_node *current, TrainData **reservation);
} TraverseConfig;

track_node *traverseReservation(TrainGlobal *train_global, TrainData *train_data, TraverseConfig *config) {
	TrainData **track_reservation = train_global->track_reservation;
	TrainData **recovery_reservation = train_global->recovery_reservation;
	track_node *track_nodes = train_global->track_nodes;
	track_node *current = &(track_nodes[config->landmark_id]);
	track_node *result;

	// IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 - 1,
		       // WIDTH_DEBUG * train_data->index, "normal: 0x%x", config->normal);

	result = config->normal(train_data, current, track_reservation);
	if(result != NULL) return result;
	result = config->recovery(train_data, current, recovery_reservation);
	if(result != NULL) return result;

	while(current->type != NODE_BRANCH) {
		if((config->distance < 0 && config->num_sensor <= 0) || current->type == NODE_EXIT) {
			return NULL;
		}

		// Deduct current the distance to the next landmark
		config->distance -= current->edge[DIR_AHEAD].dist;

		// Set current to the next
		current = current->edge[DIR_AHEAD].dest;
		config->num_sensor -= current->type == NODE_SENSOR ? 1 : 0;

		result = config->normal(train_data, current, track_reservation);
		if(result != NULL) return result;
		result = config->recovery(train_data, current, recovery_reservation);
		if(result != NULL) return result;
	}

	int switch_state = train_global->switch_table[switchIdToIndex(current->num)];
	int direction = switch_state == SWITCH_CUR ? DIR_CURVED : DIR_STRAIGHT;

	TraverseConfig new_config;
	memcpy(&new_config, config, sizeof(TraverseConfig));
	new_config.landmark_id = current->edge[direction].dest->index;
	new_config.distance = config->distance - current->edge[direction].dist;
	result = traverseReservation(train_global, train_data, &new_config);
	if(result != NULL) return result;

	if(config->take_branch > 0) {
		direction = direction == DIR_STRAIGHT ? DIR_CURVED : DIR_STRAIGHT;
		memcpy(&new_config, config, sizeof(TraverseConfig));
		new_config.landmark_id = current->edge[direction].dest->index;
		new_config.distance = config->distance - current->edge[direction].dist;
		new_config.take_branch--;
		result = traverseReservation(train_global, train_data, &new_config);
		if(result != NULL) return result;
	}
	return NULL;
}

track_node *rejectBothWay(TrainData *train_data, track_node *current, TrainData **reservation) {
	if(reservation[current->index] != NULL && reservation[current->index] != train_data) {
		return current;
	}
	if(reservation[current->reverse->index] != NULL &&
	   reservation[current->reverse->index] != train_data) {
		return current->reverse;
	}
	return NULL;
}

track_node *rejectReverse(TrainData *train_data, track_node *current, TrainData **reservation) {
	if(reservation[current->reverse->index] != NULL &&
	   reservation[current->reverse->index] != train_data) {
		return current->reverse;
	}
	return NULL;
}

track_node *reserveClear(TrainData *train_data, track_node *current, TrainData **reservation) {
	if (reservation[current->index] == train_data) {
		reservation[current->index] = NULL;
	}
	// if (reservation[current->reverse->index] == train_data) {
		// reservation[current->reverse->index] = NULL;
	// }
	return NULL;
}

track_node *reserveOneWay(TrainData *train_data, track_node *current, TrainData **reservation) {
	reservation[current->index] = train_data;
	return NULL;
}

track_node *reserveNothing(TrainData *train_data, track_node *current, TrainData **reservation) {
	return NULL;
}

inline TrainMsgType handleTrackReserve(TrainGlobal *train_global, ReservationMsg *reservation_msg) {
	TrainData* train_data = reservation_msg->train_data;
	int landmark_id = reservation_msg->landmark_id;
	int distance = reservation_msg->distance;
	int num_sensor = reservation_msg->num_sensor;
	int train_id = train_data->id;
	TraverseConfig config;

	// Check track availability
	track_node *conflict_node = NULL;
	config.landmark_id = landmark_id;
	config.distance = distance;
	config.num_sensor = num_sensor;
	config.take_branch = 1;
	if(reservation_msg->type == TRACK_RESERVE) {
		config.distance += 150;
		config.normal = rejectBothWay + TEXT_REG_BASE;
		config.recovery = rejectBothWay + TEXT_REG_BASE;
	} else if(reservation_msg->type == TRACK_RECOVERY_RESERVE) {
		config.normal = rejectReverse + TEXT_REG_BASE;
		config.recovery = rejectBothWay + TEXT_REG_BASE;
	} else {
		assert(0, "Unkown reservation message type");
	}
	conflict_node = traverseReservation(train_global, train_data, &config);

	// Track avilability conflict
	if(conflict_node != NULL) {
		IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 1,
		       WIDTH_DEBUG * train_data->index, "#%dF %s -> %s   ", train_id,
		       train_global->track_nodes[landmark_id].name, conflict_node->name);
		return TRACK_RESERVE_FAIL;
	}

	// Clear previous reservation
	if(reservation_msg->type == TRACK_RESERVE) {
		config.landmark_id = train_data->reservation_record.landmark_id;
		config.distance = train_data->reservation_record.distance;
		config.num_sensor = num_sensor;
		config.take_branch = SWITCH_TOTAL;
		config.normal = reserveClear + TEXT_REG_BASE;
		config.recovery = reserveNothing + TEXT_REG_BASE;
		traverseReservation(train_global, train_data, &config);
	}
	// Clear previous recovery reservation
	if(train_data->recovery_reservation.landmark_id != -1) {
		config.landmark_id = train_data->recovery_reservation.landmark_id;
		config.distance = train_data->recovery_reservation.distance;
		config.num_sensor = num_sensor;
		config.take_branch = SWITCH_TOTAL;
		config.normal = reserveNothing + TEXT_REG_BASE;
		config.recovery = reserveClear + TEXT_REG_BASE;
		traverseReservation(train_global, train_data, &config);
		train_data->recovery_reservation.landmark_id = -1;
	}

	// Reserve for this time
	config.landmark_id = landmark_id;
	config.distance = distance;
	config.num_sensor = num_sensor;
	config.take_branch = 1;

	if(reservation_msg->type == TRACK_RESERVE) {
		config.normal = reserveOneWay + TEXT_REG_BASE;
		config.recovery = reserveNothing + TEXT_REG_BASE;
		traverseReservation(train_global, train_data, &config);
		train_data->reservation_record.landmark_id = landmark_id;
		train_data->reservation_record.distance = distance;
	} else {
		config.normal = reserveNothing + TEXT_REG_BASE;
		config.recovery = reserveOneWay + TEXT_REG_BASE;
		traverseReservation(train_global, train_data, &config);
		train_data->recovery_reservation.landmark_id = landmark_id;
		train_data->recovery_reservation.distance = distance;
	}

	IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1,
	       WIDTH_DEBUG * train_data->index, "#%dR %s + %dmm    ",
	       train_id, train_global->track_nodes[landmark_id].name, distance);

	return TRACK_RESERVE_SUCCEED;
}

void handleLocationRecovery(TrainGlobal *train_global, CenterData *center_data, int train_id, int timestamp) {
	TrainData* train_data = train_global->train_id_data[train_id];
	int time_diff = center_data->last_lost_timestamp - timestamp;
	time_diff = time_diff > 0 ? time_diff : -1 * time_diff;

	assert(train_data != NULL, "Unknown train_id");

	if (center_data->last_lost_sensor != NULL && time_diff < RECOVERY_TIME_THRESHOLD) {
		int landmark_id = center_data->last_lost_sensor->index;
		center_data->last_lost_sensor = NULL;
		CreateWithArgs(2, locationPostman, train_data->tid, landmark_id, 0, 0);
		IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 5, train_data->index * WIDTH_DEBUG, "#%d recovery to #%s  ", train_id, train_global->track_nodes[landmark_id].name);
	} else {
		IDEBUG(DB_RESERVE, train_global->com2_tid, ROW_DEBUG_1 + 5, train_data->index * WIDTH_DEBUG, "Unable to recovery #%d", train_id);
	}
}

void saveSatelliteReport(TrainGlobal *train_global, CenterData *center_data, SatelliteReport *satellite_report) {
	// Save report
	SatelliteReport *satellite_reports = center_data->satellite_reports;
	int train_index = satellite_report->train_data->index;
	memcpy(&(satellite_reports[train_index]), satellite_report, sizeof(SatelliteReport));

	// Update sensor reservation
	// TODO
}

void fetchSatelliteReport(TrainGlobal *train_global, CenterData *center_data, SatelliteReport *satellite_report, int train_index) {
	SatelliteReport *satellite_reports = center_data->satellite_reports;
	memcpy(satellite_report, &(satellite_reports[train_index]), sizeof(SatelliteReport));
}

/* Train Center */
void trainCenter(TrainGlobal *train_global) {

	int i, tid, cmd_tid, sensor_tid, result, com1_tid, com2_tid;
	TrainMsg msg;
	TrainData *train_data;
	char str_buf[1024];

	com1_tid = train_global->com1_tid;
	com2_tid = train_global->com2_tid;

	Putc(com1_tid, SYSTEM_START);

	CenterData center_data;

	/* SensorData */
	char sensor_data[SENSOR_BYTES_TOTAL];
	memset(sensor_data, 0, SENSOR_BYTES_TOTAL);
	char sensor_decoder_ids[SENSOR_DECODER_TOTAL];
	for(i = 0; i < SENSOR_DECODER_TOTAL; i++) {
		sensor_decoder_ids[i] = 'A' + i;
	}
	center_data.sensor_data = sensor_data;
	center_data.last_lost_sensor = NULL;
	center_data.last_lost_timestamp = 0;
	center_data.lost_count = 0;

	/* Initialize Train Drivers */
	TrainData *trains_data = train_global->trains_data;
	TrainData **train_id_data = train_global->train_id_data;
	SatelliteReport satellite_reports[TRAIN_MAX];
	for(i = 0; i < TRAIN_MAX; i++) {
		assert(trains_data[i].id < TRAIN_ID_MAX, "Exceed max train number");
		trains_data[i].tid = CreateWithArgs(9, trainDriver, (int)train_global, (int)&(trains_data[i]), 0, 0);
		satellite_reports[i].type = SATELLITE_REPORT;
		satellite_reports[i].train_data = &(trains_data[i]);
		satellite_reports[i].orbit_id = -1;
		satellite_reports[i].distance = -1;
		satellite_reports[i].parent_data = NULL;
	}
	center_data.satellite_reports = satellite_reports;
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
				handleSensorUpdate(train_global, &center_data, msg.sensor_msg.sensor_data);
				break;
			case CMD_SPEED:
			case CMD_GOTO:
			case CMD_REVERSE:
			case CMD_MARGIN:
				Reply(tid, NULL, 0);
				// assert(msg.cmd_msg.id < TRAIN_ID_MAX, "Exceed max train number");
				if (msg.cmd_msg.id < TRAIN_ID_MAX) {
					train_data = train_id_data[msg.cmd_msg.id];
					if(train_data != NULL) {
						CreateWithArgs(2, cmdPostman, train_data->tid, msg.type, msg.cmd_msg.value, 0);
					}
				}
				break;
			case CMD_SWITCH:
				Reply(tid, NULL, 0);
				handleSwitchCommand(&(msg.cmd_msg), train_global);
				break;
			case CMD_QUIT:
				Halt();
				break;
			case TRACK_RESERVE:
			case TRACK_RECOVERY_RESERVE:
				msg.type = handleTrackReserve(train_global, &(msg.reservation_msg));
				Reply(tid, (char *)(&(msg.type)), sizeof(TrainMsgType));
				break;
			case LOCATION_RECOVERY:
				Reply(tid, NULL, 0);
				handleLocationRecovery(train_global, &center_data, msg.location_msg.id, msg.location_msg.value);
				break;
			case SATELLITE_REPORT:
				saveSatelliteReport(train_global, &center_data, &(msg.satellite_report));
				if(msg.satellite_report.parent_data != NULL) {
					fetchSatelliteReport(train_global, &center_data, &(msg.satellite_report), msg.satellite_report.parent_data->index);
					Reply(tid, (char *)(&(msg.satellite_report)), sizeof(SatelliteReport));
				} else {
					Reply(tid, NULL, 0);
				}
				break;
			default:
				sprintf(str_buf, "Center got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}
