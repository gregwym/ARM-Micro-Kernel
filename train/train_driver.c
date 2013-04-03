#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

#define DELAY_REVERSE 200
#define RESERVE_CHECKPOINT_LEN 50
#define RESERVER_PRIORITY 7

track_node *predictNextSensor(TrainGlobal *train_global, TrainData *train_data) {
	int direction;
	track_node *checked_node = train_data->last_receive_sensor;
	checked_node = checked_node->edge[DIR_AHEAD].dest;
	while (1) {
		switch(checked_node->type) {
			case NODE_ENTER:
			case NODE_MERGE:
				checked_node = checked_node->edge[DIR_AHEAD].dest;
				break;
			case NODE_BRANCH:
				direction = train_global->switch_table[switchIdToIndex(checked_node->num)] - 33;
				checked_node = checked_node->edge[direction].dest;
				break;
			case NODE_SENSOR:
				return checked_node;
			case NODE_EXIT:
				return train_data->last_receive_sensor;
			default:
				return NULL;
		}
	}
}

track_node *findNextNode(TrainGlobal *train_global, TrainData *train_data) {
	int direction;
	switch(train_data->landmark->type) {
		case NODE_ENTER:
		case NODE_MERGE:
		case NODE_SENSOR:
			return train_data->landmark->edge[DIR_AHEAD].dest;
		case NODE_BRANCH:
			direction = train_global->switch_table[switchIdToIndex(train_data->landmark->num)] - 33;
			return train_data->landmark->edge[direction].dest;
		case NODE_EXIT:
			return train_data->landmark;
		default:
			return NULL;
	}
}

void trackReserver(TrainGlobal *train_global, TrainData *train_data, int landmark_id, int distance) {
	int center_tid = train_global->center_tid;
	int result, driver_tid;
	distance += 100;
	TrainMsgType reply;
	ReservationMsg msg;
	msg.type = train_data->action == RB_last_ss ? TRACK_RECOVERY_RESERVE : TRACK_RESERVE;
	msg.train_data = train_data;
	msg.landmark_id = landmark_id;
	msg.distance = distance;
	msg.num_sensor = 1;
	driver_tid = MyParentTid();


	// Send reservation request to the center
	// IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 6, 40 * train_data->index + 2, "Reserving %d + %dmm", landmark_id, distance);
	result = Send(center_tid, (char *)(&msg), sizeof(ReservationMsg), (char *)(&reply), sizeof(TrainMsgType));
	assert(result > 0, "Reserver fail to receive reply from center");

	// IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 6, 40 * train_data->index + 2, "Delivering %d + %dmm", landmark_id, distance);
	result = Send(driver_tid, (char *)(&reply), sizeof(TrainMsgType), NULL, 0);
	assert(result == 0, "Reserver fail to send reservation result to the driver");
}

void relocationRequest(TrainGlobal *train_global, TrainData *train_data) {
	int center_tid = train_global->center_tid;
	int result;

	LocationMsg location_msg;
	location_msg.type = LOCATION_RECOVERY;
	location_msg.id = train_data->id;
	location_msg.value = getTimerValue(TIMER3_BASE);

	result = Send(center_tid, (char *)(&location_msg), sizeof(LocationMsg), NULL, 0);
	assert(result == 0, "Relocation failed to send to center");
}

void routeRequest(TrainGlobal *train_global, TrainData *train_data, track_node *dest) {
	int result;
	RouteMsg msg;
	msg.type = FIND_ROUTE;
	msg.train_data = train_data;
	msg.destination = dest;
	result = Send(train_global->route_server_tid, (char *)(&msg), sizeof(RouteMsg), NULL, 0);
	assert(result == 0, "routeDelivery fail to deliver route");
}

inline void setTrainSpeed(int train_id, int speed, int com1_tid) {
	char cmd[2];
	cmd[0] = speed;
	cmd[1] = train_id;
	Puts(com1_tid, cmd, 2);
}

inline int distToCM(int dist) {
	return (dist >> DIST_SHIFT) / 10;
}

int calcDistance(track_node *src, track_node *dest, int depth) {
	// If found return the distance
	if(src == dest) {
		return 0;
	}
	if(depth == 0) {
		return -1;
	}

	int straight = -1;
	int curved = -1;

	switch(src->type) {
		case NODE_ENTER:
		case NODE_SENSOR:
		case NODE_MERGE:
			straight = calcDistance(src->edge[DIR_AHEAD].dest, dest, depth - 1);
			if(straight >= 0) return straight + src->edge[DIR_AHEAD].dist;
			break;
		case NODE_BRANCH:
			straight = calcDistance(src->edge[DIR_STRAIGHT].dest, dest, depth - 1);
			if(straight >= 0) return straight + src->edge[DIR_STRAIGHT].dist;
			curved = calcDistance(src->edge[DIR_CURVED].dest, dest, depth - 1);
			if(curved >= 0) return curved + src->edge[DIR_CURVED].dist;
			break;
		default:
			break;
	}

	return -1;
}

int gotoNode(track_node *src, track_node *dest, TrainGlobal *train_global, int depth) {
	if(src == dest) {
		return 1;
	}
	if(depth == 0) {
		return 0;
	}

	int straight = -1;
	int curved = -1;

	switch(src->type) {
		case NODE_ENTER:
		case NODE_SENSOR:
		case NODE_MERGE:
			return gotoNode(src->edge[DIR_AHEAD].dest, dest, train_global, depth - 1);
			break;
		case NODE_BRANCH:
			straight = gotoNode(src->edge[DIR_STRAIGHT].dest, dest, train_global, depth - 1);
			if (straight) {
				if (train_global->switch_table[switchIdToIndex(src->num)] != SWITCH_STR) {
					CreateWithArgs(2, switchChanger, (int)train_global, src->num, SWITCH_STR, 0);
				}
				return straight;
			}
			curved = gotoNode(src->edge[DIR_CURVED].dest, dest, train_global, depth - 1);
			if (curved) {
				if (train_global->switch_table[switchIdToIndex(src->num)] != SWITCH_CUR) {
					CreateWithArgs(2, switchChanger, (int)train_global, src->num, SWITCH_CUR, 0);
				}
				return curved;
			}
			return 0;
			break;
		default:
			return 0;
	}
}

int getNextNodeDist(track_node *cur_node, char *switch_table, int *direction) {
	int ret;
	switch(cur_node->type) {
		case NODE_ENTER:
		case NODE_SENSOR:
		case NODE_MERGE:
			*direction = DIR_AHEAD;
			ret = cur_node->edge[DIR_AHEAD].dist;
			return ret;
		case NODE_BRANCH:
			*direction = switch_table[switchIdToIndex(cur_node->num)] - 33;
			ret = cur_node->edge[*direction].dist;
			return ret;
		default:
			return -1;
	}
}

StopType exitCheck(TrainGlobal *train_global, TrainData *train_data, int stop_distance) {
	int distance = 0;
	int direction = 0;

	track_node *checked_node = train_data->landmark;

	while (distance < (stop_distance + train_data->ahead_lm)) {
		switch(checked_node->type) {
			case NODE_SENSOR:
			case NODE_MERGE:
			case NODE_ENTER:
				distance += (checked_node->edge[DIR_AHEAD].dist << DIST_SHIFT);
				checked_node = checked_node->edge[DIR_AHEAD].dest;
				break;
			case NODE_BRANCH:
				direction = train_global->switch_table[switchIdToIndex(checked_node->num)] - 33;
				distance += (checked_node->edge[direction].dist << DIST_SHIFT);
				checked_node = checked_node->edge[direction].dest;
				break;
			case NODE_EXIT:
				train_data->exit_node = checked_node;
				return Entering_Exit;
				break;
			default:
				bwprintf(COM2, "node type: %d\n", checked_node->type);
				assert(0, "stopCheck function cannot find a valid node type(1)");
				break;
		}
	}
	return None;
}

StopType destCheck(TrainGlobal *train_global, TrainData *train_data, int stop_distance, track_node *stop_node, int offset) {
	assert(train_data->stop_type == None && (train_data->action == Goto_Dest || train_data->action == Goto_Merge), "stop type or action invalid in destCheck");
	assert(stop_node != NULL, "stop node is NULL in destCheck");
	assert(train_data->check_point != -1, "check point is -1 in destCheck");

	int distance = 0;
	int check_point = train_data->check_point;
	track_node **route = train_data->route;

	for (; (distance + offset) < (stop_distance + train_data->ahead_lm) && check_point < TRACK_MAX; check_point++) {
		if (stop_node == route[check_point]) {
			if (train_data->action == Goto_Dest) {
				return Entering_Dest;
			} else if (train_data->action == Goto_Merge) {
				return Entering_Merge;
			}
		}
		if (route[check_point]->type == NODE_EXIT) {
			train_data->exit_node = route[check_point];
			return Entering_Exit;
		}
		if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
				distance += ((route[check_point]->edge[DIR_STRAIGHT].dist) << DIST_SHIFT);
			} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
				distance += ((route[check_point]->edge[DIR_CURVED].dist) << DIST_SHIFT);
			} else {
				assert(0, "die ah sitra branch cannot find a dest");
			}
		} else {
			distance += (route[check_point]->edge[DIR_AHEAD].dist << DIST_SHIFT);
		}
	}
	return None;
}

void trainSecretary() {
	int parent_tid = MyParentTid();
	char ch = '1';
	while(1) {
		Delay(2);
		Send(parent_tid, &ch, 0, NULL, 0);
	}
}

void changeNextSW(TrainGlobal *train_global, TrainData *train_data, int check_point, char *switch_table, int threshold) {
	// char cmd[2];
	int distance = 0;
	// check_point += 1;
	track_node **route = train_data->route;
	for (; check_point >= 0 && check_point < TRACK_MAX && distance < 1050; check_point++) {
		if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
					CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
					IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
				}
				distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
			} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
					CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
					IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
				}
				distance += route[check_point]->edge[DIR_CURVED].dist;
			} else {
				assert(0, "fail to change switch");
			}
		} else if (route[check_point]->type == NODE_MERGE && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] != route[check_point]->edge[DIR_AHEAD].dest) {
				if (route[check_point]->reverse->edge[DIR_STRAIGHT].dest == route[check_point + 1]) {
					if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
						IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					}
					distance += (route[check_point]->reverse->edge)[DIR_STRAIGHT].dist;
				} else if (route[check_point]->reverse->edge[DIR_CURVED].dest == route[check_point + 1]) {
					if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
						IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					}
					distance += (route[check_point]->reverse->edge)[DIR_CURVED].dist;
				} else {
					assert(0, "fail to change switch (1)");
				}
				if (route[check_point]->edge[DIR_AHEAD].dest->type == NODE_MERGE && route[check_point]->edge[DIR_AHEAD].dist < 240) {
					if (route[check_point]->edge[DIR_AHEAD].dest->reverse->edge[DIR_STRAIGHT].dest == route[check_point]->reverse) {
						if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_STR) {
							CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_STR, 0);
							IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
						}
					} else {
						if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_CUR) {
							CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_CUR, 0);

							IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
						}
					}
				}
			} else {
				distance += route[check_point]->edge[DIR_AHEAD].dist;
			}
		} else {
			distance += route[check_point]->edge[DIR_AHEAD].dist;
		}
	}
}

int find_stop_dist(TrainData *train_data) {
	int i = -1;
	if (train_data->velocity >= train_data->velocities[7]) {
		for (i = 7; i < 15; i++) {
			if (train_data->velocities[i] > train_data->velocity) {
				break;
			}
		}
	} else {
		for (i = 1; i < 7; i++) {
			if (train_data->velocities[i] > train_data->velocity) {
				break;
			}
		}
	}
	i -= 1;
	return train_data->stop_dist[i] + (train_data->ht_length[train_data->direction] << DIST_SHIFT);
}


void reserveInRecovery(TrainGlobal *train_global, TrainData *train_data) {
	// bwprintf(COM2, "Recovery %d + %dmm\n", train_data->landmark->index, (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
	IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 7, 40 * train_data->index + 2, "Recovery %d + %dmm", train_data->last_receive_sensor->reverse->index, (find_stop_dist(train_data)) >> DIST_SHIFT);
	CreateWithArgs(RESERVER_PRIORITY, trackReserver, (int)train_global,
				   (int)train_data, train_data->last_receive_sensor->reverse->index,
				   (find_stop_dist(train_data)) >> DIST_SHIFT);
	train_data->waiting_for_reserver = TRUE;
	train_data->last_reservation_time = train_data->timer;
	// train_data->dist_since_last_rs = 0;
}

void reserveInRunning(TrainGlobal *train_global, TrainData *train_data) {
	// bwprintf(COM2, "Running %d + %dmm\n", train_data->landmark->index, (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
	track_node * sensor_for_reserve = train_data->sensor_for_reserve;
	if (train_data->action != RB_last_ss) {
		int last_sensor_dist = -1;
		if(sensor_for_reserve != NULL) {
			last_sensor_dist = calcDistance(sensor_for_reserve, train_data->landmark, 12);
		}
		int reserve_start = train_data->landmark->index;
		int reserve_dist = (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT;

		if(last_sensor_dist > 0) {
			reserve_start = sensor_for_reserve->index;
			reserve_dist += last_sensor_dist;
		}

		IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 7, 40 * train_data->index + 2, "Running %d + %dmm, time: %u", reserve_start, reserve_dist, 0xffffffff - train_data->timer);
		CreateWithArgs(RESERVER_PRIORITY, trackReserver, (int)train_global,
					   (int)train_data, reserve_start, reserve_dist);
		train_data->waiting_for_reserver = TRUE;
		train_data->last_reservation_time = train_data->timer;
	}
	// train_data->dist_since_last_rs = 0;
}


// return check point forward by 1, and adjust it if it does not match current landmark
// return -1 if current landmark 6is not in the route
int updateCheckPoint(TrainData *train_data, track_node **route, int check_point, int route_start) {
	assert(check_point != -1, "check point is -1 in updateCheckPoint");
	if (check_point < 0) {
		return -1;
	} else if (check_point + 1 < TRACK_MAX) {
		check_point += 1;
		if (route[check_point] != train_data->landmark) {
			for (;route_start < TRACK_MAX; route_start++) {
				if (route[route_start] == train_data->landmark) {
					return route_start;
				}
			}
			return -1;
		} else {
			return check_point;
		}
	} else {
		for (;route_start < TRACK_MAX; route_start++) {
			if (route[route_start] == train_data->landmark) {
				return route_start;
			}
		}
		return -1;
	}
}

void reverseTrainAndLandmark(TrainGlobal *train_global, TrainData *train_data, char *switch_table) {
	int train_index = train_data->index;
	int direction;

	setTrainSpeed(train_data->id, TRAIN_REVERSE, train_global->com1_tid);
	train_data->reverse_protect = TRUE;
	train_data->reverse_delay = TRUE;
	train_data->untrust_sensor = predictNextSensor(train_global, train_data);

	if (train_data->direction == FORWARD) {
		train_data->direction = BACKWARD;
	} else {
		train_data->direction = FORWARD;
	}
	if (train_data->landmark->type != NODE_EXIT) {
		track_node *tmp = findNextNode(train_global, train_data);
		train_data->landmark = tmp->reverse;
		// train_data->forward_distance = (getNextNodeDist(train_data->landmark, switch_table, &direction) << DIST_SHIFT);
		train_data->predict_dest = findNextNode(train_global, train_data);
		train_data->ahead_lm = train_data->forward_distance - train_data->ahead_lm;
	} else {
		train_data->landmark = train_data->landmark->reverse;
		train_data->forward_distance = (getNextNodeDist(train_data->landmark, switch_table, &direction) << DIST_SHIFT);
		train_data->predict_dest = train_data->landmark->edge[direction].dest;
	}
	train_data->sensor_for_reserve = train_data->landmark;

	int com2_tid = train_global->com2_tid;
	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_1, "%s  ", train_data->landmark->name);
	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_1, "%s  ", train_data->predict_dest->name);
}

//update current landmark
void updateCurrentLandmark(TrainGlobal *train_global, TrainData *train_data, track_node *sensor_node, char *switch_table) {
	int train_index = train_data->index;
	int direction;

	if (sensor_node != NULL) {
		train_data->landmark = sensor_node;
	} else {
		train_data->landmark = findNextNode(train_global, train_data);
	}

	if (train_data->landmark->type != NODE_EXIT) {
		train_data->ahead_lm = 0;
		train_data->forward_distance = getNextNodeDist(train_data->landmark, switch_table, &direction);
		if (train_data->forward_distance >= 0) {
			train_data->forward_distance = train_data->forward_distance << DIST_SHIFT;
			train_data->predict_dest = train_data->landmark->edge[direction].dest;
		} else {
			DEBUG(DB_ROUTE, "forward_distance -1 %s", train_data->landmark->name);
		}
	} else {
		train_data->ahead_lm = 0;
		train_data->forward_distance = 0;
	}

	int com2_tid = train_global->com2_tid;
	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_1, "%s  ", train_data->landmark->name);
	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_1, "%s  ", train_data->predict_dest->name);

}

void changeSpeed(TrainGlobal *train_global, TrainData *train_data, int new_speed) {
	if (new_speed == 0) {
		IDEBUG(DB_ROUTE, 4, 55, 2 + train_data->index * 40, "stoptype: %d " , train_data->stop_type);
	}
	if (train_data->speed % 16 < new_speed % 16) {
		if (train_data->speed % 16 != 0) {
			train_data->old_speed = train_data->speed;
		}
		train_data->speed = new_speed;
		setTrainSpeed(train_data->id, new_speed, train_global->com1_tid);
		if (train_data->reverse_delay) {
			train_data->acceleration_step = 0;
			train_data->acceleration_alarm = getTimerValue(TIMER3_BASE) - 600;
		} else {
			train_data->acceleration_step = train_data->velocity * 5 / train_data->velocities[14] + 1;
			train_data->acceleration_alarm = getTimerValue(TIMER3_BASE) - train_data->acceleration_time;
		}
		assert(train_data->acceleration_step < 6, "acceleration step exceeds");
		train_data->acceleration = train_data->accelerations[train_data->acceleration_step];
	} else if (train_data->speed % 16 > new_speed % 16 || new_speed % 16 == 0) {
		train_data->acceleration_step = 0;
		train_data->acceleration_alarm = 0;
		if (train_data->speed % 16 != 0) {
			train_data->old_speed = train_data->speed;
		}
		train_data->speed = new_speed;
		setTrainSpeed(train_data->id, new_speed, train_global->com1_tid);
		train_data->acceleration = train_data->deceleration;
	}
}

void updateTrainStatus(TrainGlobal *train_global, TrainData *train_data) {
	// handle acceleration
	if (train_data->acceleration_alarm > train_data->timer) {
		switch(train_data->acceleration_step) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				train_data->acceleration_step++;
				train_data->acceleration_alarm = getTimerValue(TIMER3_BASE) - train_data->acceleration_time;
				train_data->acceleration = train_data->accelerations[train_data->acceleration_step];
				break;
			case 5:
				train_data->acceleration_alarm = 0;
				train_data->acceleration_step = -1;
				break;
			default:
				assert(0, "invalid acceleration_step");
		}
	}

	// handle velocity
	train_data->velocity = train_data->velocity + (train_data->prev_timer - train_data->timer) * train_data->acceleration;
	if (train_data->acceleration > 0) {
		if (train_data->velocity > train_data->velocities[train_data->speed % 16]) {
			train_data->velocity = train_data->velocities[train_data->speed % 16];
			train_data->acceleration = 0;
			train_data->acceleration_alarm = 0;
			train_data->acceleration_step = -1;
		}
	} else if (train_data->acceleration < 0) {
		if (train_data->velocity < train_data->velocities[train_data->speed % 16]) {
			train_data->velocity = train_data->velocities[train_data->speed % 16];
			train_data->acceleration = 0;
			if (train_data->velocity == 0) {
				train_data->v_to_0 = 1;
			}
		}
	}

	// location update
	train_data->ahead_lm = train_data->ahead_lm + train_data->velocity * (train_data->prev_timer - train_data->timer);
	// train_data->dist_since_last_rs += train_data->velocity * (train_data->prev_timer - train_data->timer);

	if (train_data->ahead_lm > train_data->forward_distance) {
		updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table);
		if (train_data->action == Goto_Dest || train_data->action == Goto_Merge) {
			int tmp = updateCheckPoint(train_data, train_data->route, train_data->check_point, train_data->route_start);
			if (tmp != -1) {
				train_data->check_point = tmp;
				if (train_data->landmark->type == NODE_SENSOR) {
					changeNextSW(train_global, train_data, train_data->check_point, train_global->switch_table, find_stop_dist(train_data) >> DIST_SHIFT);
				}
			// assert(check_point != -1, "check_point is -1");
			} else {
				train_data->check_point = -1;
				train_data->action = Off_Route;
				IDEBUG(DB_ROUTE, 4, ROW_DEBUG_2 + 2, 2 + train_data->index * 40, "off route: %s   ", train_data->landmark->name);
			}
		}

		// lost checking
		if (train_data->landmark->type == NODE_SENSOR && train_data->last_receive_sensor != NULL) {
			train_data->predict_sensor_num++;
			if (train_data->predict_sensor_num > 1) {
				changeSpeed(train_global, train_data, 16);
				train_data->stop_type = Stopped;
				// uiprintf(train_global->com2_tid, 51, 2, "train %d is trapped!", train_data->id);
				CreateWithArgs(2, relocationRequest, (int)train_global, (int)train_data, 0, 0);
			}
		}
	}
}

void updateReservation(TrainGlobal *train_global, TrainData *train_data) {
	if (train_data->last_reservation_time - train_data->timer > 400) {
		if (train_data->waiting_for_reserver) {
			changeSpeed(train_global, train_data, 0);
			// train_data->acceleration = train_data->deceleration;
			train_data->stop_type = RB_slowing;
			// train_data->dist_since_last_rs = 0;
		} else {
			if (train_data->velocity != 0) {
				if (train_data->action != RB_last_ss && train_data->action != Off_Route) {
					reserveInRunning(train_global, train_data);
				}
			} else {
				if (train_data->action == RB_last_ss) {
					reserveInRecovery(train_global, train_data);
				} else {
					// reserveInRunning(train_global, train_data);
					reserveInRunning(train_global, train_data);
				}
			}
		}
	}
}

void stopChecking(TrainGlobal *train_global, TrainData *train_data) {
	if (train_data->speed % 16 != 0) {
		assert(train_data->acceleration >= 0, "already decelerating in stop checking");
		assert(train_data->stop_type == None, "stop type is not None in stop checking");
		if (train_data->action == Free_Run || train_data->action == Off_Route) {
			train_data->stop_type = exitCheck(train_global, train_data,
										find_stop_dist(train_data));
			if (train_data->stop_type == Entering_Exit) {
				changeSpeed(train_global, train_data, 0);
				// IDEBUG(DB_ROUTE, 4, ROW_DEBUG_2 + 5, COLUMN_FIRST, "stop, %s, %d v: %d  sd: %d          ", train_data->landmark->name, train_data->ahead_lm >> 18, train_data->velocity, find_stop_dist(train_data) >> 18);
			}
		} else if (train_data->action == Goto_Merge) {
			train_data->stop_type = destCheck(train_global, train_data,
										find_stop_dist(train_data), train_data->reverse_node, train_data->reverse_node_offset);
			if (train_data->stop_type == Entering_Merge) {
				changeSpeed(train_global, train_data, 0);
				if (train_global->switch_table[switchIdToIndex(train_data->merge_node->num)] != train_data->merge_state) {
					CreateWithArgs(2, switchChanger, (int)train_global, train_data->merge_node->num, train_data->merge_state, 0);
				}
				if (train_data->merge_node->edge[DIR_AHEAD].dest->type == NODE_MERGE) {
					if (train_data->merge_node->edge[DIR_AHEAD].dest->reverse->edge[DIR_STRAIGHT].dest
							== train_data->merge_node->reverse) {
						if (train_global->switch_table[switchIdToIndex(train_data->merge_node->edge[DIR_AHEAD].dest->num)] != SWITCH_STR) {
							CreateWithArgs(2, switchChanger, (int)train_global, train_data->merge_node->edge[DIR_AHEAD].dest->num, SWITCH_STR, 0);
						}
					} else {
						if (train_global->switch_table[switchIdToIndex(train_data->merge_node->edge[DIR_AHEAD].dest->num)] != SWITCH_CUR) {
							CreateWithArgs(2, switchChanger, (int)train_global, train_data->merge_node->edge[DIR_AHEAD].dest->num, SWITCH_CUR, 0);
						}
					}
				}
			}
		} else if (train_data->action == Goto_Dest) {
			train_data->stop_type = destCheck(train_global, train_data,
										find_stop_dist(train_data), train_data->stop_node, 0);
			if (train_data->stop_type == Entering_Dest) {
				changeSpeed(train_global, train_data, 0);
			}
		}
		if (train_data->stop_type != None) {
			IDEBUG(DB_ROUTE, train_global->com2_tid, 52, 2 + train_data->index * 40, "stop at: %s, %d  ", train_data->landmark->name, train_data->ahead_lm >> DIST_SHIFT);
		}
	}
}

void changeState(TrainGlobal *train_global, TrainData *train_data) {
	int result;
	if (train_data->v_to_0 && !train_data->waiting_for_reserver) {
		train_data->v_to_0 = FALSE;
		uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_3, "%d ", train_data->stop_type);

		switch(train_data->stop_type) {
			case Entering_Exit:
				if (train_data->stop_node != NULL) {
					reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
					CreateWithArgs(2, routeRequest, (int)train_global, (int)train_data, (int)train_data->stop_node, 0);
					train_data->check_point = -1;
					train_data->action = Off_Route;
					train_data->stop_type = Finding_Route;
				} else {
					train_data->stop_type = None;
				}
				break;

			case Entering_Dest:
				train_data->stop_type = None;
				train_data->stop_node = NULL;
				train_data->action = Free_Run;
				uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "    ");
				break;

			case Entering_Merge:

				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
				// reserveInReversing(train_global, train_data);
				train_data->reverse_node = NULL;
				train_data->reverse_node_offset = 0;
				train_data->merge_node = NULL;

				CreateWithArgs(2, routeRequest, (int)train_global, (int)train_data, (int)train_data->stop_node, 0);
				train_data->check_point = -1;
				train_data->action = Off_Route;
				train_data->stop_type = Finding_Route;
				break;

			case Reversing:
				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
				// reserveInReversing(train_global, train_data);
				train_data->stop_type = RB_go;
				if (train_data->action != Free_Run) {
					CreateWithArgs(2, routeRequest, (int)train_global, (int)train_data, (int)train_data->stop_node, 0);
					train_data->check_point = -1;
					train_data->action = Off_Route;
					train_data->stop_type = Finding_Route;
				}
				break;

			case Stopped:
				train_data->stop_type = None;
				break;

			case RB_slowing:
				// Wait for last reservation result, then enter recovery mode
				result = gotoNode(train_data->predict_dest->reverse, train_data->last_receive_sensor->reverse, train_global, 8);

				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);

				uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_CURRENT + 2, COLUMN_DATA_3, "%s->%s  ", train_data->landmark->name, train_data->last_receive_sensor->reverse->name);
				train_data->stop_type = RB_go;
				train_data->action = RB_last_ss;
				// train_data->stop_type = RB_changing_to_lost;
				break;

			case RB_go:
				assert(0, "should stop in RB_last_ss or RB_go");
				break;
			case Finding_Route:
			default:
				break;
		}
	}
}


void trainDriver(TrainGlobal *train_global, TrainData *train_data) {
	track_node *track_nodes = train_global->track_nodes;
	int tid, result, need_recalculate;
	int train_id = train_data->id;
	int train_index = train_data->index;


	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;
	TrainMsg msg;
	char str_buf[1024];

	// initialize start position
	updateCurrentLandmark(train_global, train_data,
	                      &(track_nodes[train_data->reservation_record.landmark_id]),
	                      train_global->switch_table);

	char *buf_cursor = str_buf;

	// Initialize train speed
	setTrainSpeed(train_id, train_data->speed, com1_tid);

	int secretary_tid = Create(2, trainSecretary);

	int cnt = 0;
	TrainDirection prev_reserve_dict = FORWARD;

	// int reversing = 0;
	// track_node *merge_node = NULL;
	// int simple_reverse = 0;

	char cmd[2];
	cmd[1] = train_id;

	// iprintf(com2_tid, 10, "\e[s\e[20;2Hcom2: %d \e[u", com2_tid);

	Delay(1000);

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");
		if (tid == secretary_tid) {
			Reply(tid, NULL, 0);
			train_data->timer = getTimerValue(TIMER3_BASE);

			updateTrainStatus(train_global, train_data);
			updateReservation(train_global, train_data);
			stopChecking(train_global, train_data);
			changeState(train_global, train_data);
			train_data->prev_timer = train_data->timer;

			cnt++;
			if (cnt == 8) cnt = 0;

			if (!cnt) {
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_STATUS,
				         COLUMN_DATA_1, "%d  sp: %d ", train_data->velocity, train_data->speed);
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_2, "%d  ", train_data->ahead_lm >> DIST_SHIFT);
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_2, "%d  ", (train_data->forward_distance - train_data->ahead_lm) >> DIST_SHIFT);
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_3, "%d ", train_data->action);
			}
			continue;
		}
		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				changeSpeed(train_global, train_data, msg.cmd_msg.value);
				if (train_data->speed % 16 == 0) {
					train_data->stop_type = Stopped;
				}
				break;

			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				changeSpeed(train_global, train_data, 0);
				train_data->stop_type = Reversing;
				break;

			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				if (train_data->reverse_protect) {
					if (train_data->untrust_sensor == &(track_nodes[msg.location_msg.id])) {
						train_data->untrust_sensor = NULL;
						train_data->reverse_protect = FALSE;
						continue;
					} else {
						train_data->untrust_sensor = NULL;
						train_data->reverse_protect = FALSE;
					}
				}

				if (train_data->action == RB_last_ss) {
					uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_CURRENT + 3, COLUMN_DATA_3, "%s  ", track_nodes[msg.location_msg.id].name);
					if (train_data->stop_node != NULL) {
						train_data->action = Off_Route;
					} else {
						train_data->action = Free_Run;
					}
				}

				train_data->last_receive_sensor = &(track_nodes[msg.location_msg.id]);
				train_data->sensor_for_reserve = &(track_nodes[msg.location_msg.id]);
				train_data->predict_sensor_num = 0;
				need_recalculate = FALSE;

				updateCurrentLandmark(train_global, train_data, &(track_nodes[msg.location_msg.id]), train_global->switch_table);
				if (train_data->action == Off_Route && train_data->stop_type == None) {
					need_recalculate = TRUE;
				} else if (train_data->action == Goto_Dest || train_data->action == Goto_Merge) {
					train_data->check_point = updateCheckPoint(train_data, train_data->route, train_data->check_point, train_data->route_start);
					if (train_data->check_point == -1) {
						need_recalculate = TRUE;
					} else {
						changeNextSW(train_global, train_data, train_data->check_point, train_global->switch_table, find_stop_dist(train_data) >> DIST_SHIFT);
					}
				}

				if (need_recalculate) {
					CreateWithArgs(2, routeRequest, (int)train_global, (int)train_data, (int)(train_data->stop_node), 0);
					changeSpeed(train_global, train_data, 0);
					train_data->check_point = -1;
					train_data->action = Off_Route;
					train_data->stop_type = Finding_Route;
				}

				break;
			case CMD_GOTO:
				Reply(tid, NULL, 0);

				CreateWithArgs(2, routeRequest, (int)train_global, (int)train_data, (int)(&(track_nodes[msg.location_msg.value])), 0);
				if (train_data->speed % 16  != 0) {
					changeSpeed(train_global, train_data, 0);
				}
				train_data->check_point = -1;
				train_data->action = Off_Route;
				train_data->stop_type = Finding_Route;

				break;
			case CMD_MARGIN:
				Reply(tid, NULL, 0);
				train_data->margin = msg.cmd_msg.value;
				break;
			case TRACK_RESERVE_FAIL:
				Reply(tid, NULL, 0);
				train_data->waiting_for_reserver = FALSE;
				if (train_data->stop_type != RB_go) {
					// assert(prev_reserve_dict == train_data->direction, "bidirectional reservation FAILURE!");
					prev_reserve_dict = train_data->direction;
					train_data->stop_type = RB_slowing;
					changeSpeed(train_global, train_data, 0);
					IDEBUG(DB_ROUTE, 4, 53, 2 + train_data->index * 40, "re stop: %s, %d    ", train_data->landmark->name, train_data->ahead_lm >> 18);
					IDEBUG(DB_ROUTE, 4, 54, 5 + train_data->index * 40, "__%s  ", train_data->direction == FORWARD ? "F" : "B");
				}
				break;
			case TRACK_RESERVE_SUCCEED:
				Reply(tid, NULL, 0);
				train_data->waiting_for_reserver = FALSE;
				if (train_data->stop_type == RB_go || train_data->stop_type == RB_slowing) {
					changeSpeed(train_global, train_data, train_data->old_speed);
					train_data->stop_type = None;
					// reserveInRunning(train_global, train_data);
				}
				break;
			case NEED_REVERSE:
				Reply(tid, NULL, 0);
				changeSpeed(train_global, train_data, 0);
				train_data->check_point = -1;
				train_data->action = Off_Route;
				train_data->stop_type = Reversing;
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				break;
			case GOTO_MERGE:
				Reply(tid, NULL, 0);

				train_data->action = Goto_Merge;
				train_data->check_point = train_data->route_start;
				changeNextSW(train_global, train_data, train_data->check_point, train_global->switch_table, find_stop_dist(train_data) >> DIST_SHIFT);
				train_data->stop_type = RB_go;
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				break;
			case GOTO_DEST:
				Reply(tid, NULL, 0);

				train_data->action = Goto_Dest;
				train_data->check_point = train_data->route_start;
				changeNextSW(train_global, train_data, train_data->check_point, train_global->switch_table, find_stop_dist(train_data) >> DIST_SHIFT);
				train_data->stop_type = RB_go;
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				break;
			default:
				sprintf(str_buf, "Driver got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
		buf_cursor = str_buf;
	}
}
