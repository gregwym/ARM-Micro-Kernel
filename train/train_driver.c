#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

#define DELAY_REVERSE 200
#define RESERVE_CHECKPOINT_LEN 50
#define RESERVER_PRIORITY 7

void trainReverser(int train_id, int new_speed, int com1_tid, int delay_time) {
	char cmd[2];
	cmd[0] = 0;
	cmd[1] = train_id;
	if (delay_time != 0) {
		Puts(com1_tid, cmd, 2);

		// Delay 2s then turn off the solenoid
		Delay(delay_time / 20);
	}

	// Reverse
	cmd[0] = TRAIN_REVERSE;
	Puts(com1_tid, cmd, 2);

	// Re-accelerate
	cmd[0] = new_speed;
	Puts(com1_tid, cmd, 2);
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
	IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 5, 40 * train_data->index + 2, "Reserving %d + %dmm", landmark_id, distance);
	result = Send(center_tid, (char *)(&msg), sizeof(ReservationMsg), (char *)(&reply), sizeof(TrainMsgType));
	assert(result > 0, "Reserver fail to receive reply from center");

	IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 6, 40 * train_data->index + 2, "Delivering %d + %dmm", landmark_id, distance);
	result = Send(driver_tid, (char *)(&reply), sizeof(TrainMsgType), NULL, 0);
	assert(result == 0, "Reserver fail to send reservation result to the driver");
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
					train_global->switch_table[switchIdToIndex(src->num)] = SWITCH_STR;
					CreateWithArgs(2, switchChanger, src->num, SWITCH_STR, train_global->com1_tid, train_global->com2_tid);
				}
				return straight;
			}
			curved = gotoNode(src->edge[DIR_CURVED].dest, dest, train_global, depth - 1);
			if (curved) {
				if (train_global->switch_table[switchIdToIndex(src->num)] != SWITCH_CUR) {
					train_global->switch_table[switchIdToIndex(src->num)] = SWITCH_CUR;
					CreateWithArgs(2, switchChanger, src->num, SWITCH_CUR, train_global->com1_tid, train_global->com2_tid);
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
	assert(train_data->stop_node != NULL, "stop node is NULL in destCheck");
	assert(train_data->check_point != -1, "check point is -1 in destCheck");
	
	int distance = 0;
	int check_point = train_data->check_point;
	track_node **route = train_data->route;

	for (; (distance + offset) < (stop_distance + train_data->ahead_lm) && check_point < TRACK_MAX; check_point++) {
		if (train_data->stop_node == route[check_point]) {
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

inline void dijkstra_update(Heap *dist_heap, HeapNode *heap_nodes, track_node **previous,
                            track_node *u, track_node *neighbour, int alter_dist) {
	// Find the old distance
	HeapNode *heap_node = &(heap_nodes[neighbour->index]);
	int old_dist = heap_node->key;

	// If it is a node not in the heap, or has a shorter alter distance
	if(old_dist == -1 || alter_dist < old_dist) {
		// Assign the new distance and previous
		heap_node->key = alter_dist;
		previous[neighbour->index] = u;
		if(old_dist == -1) minHeapInsert(dist_heap, heap_node);
		else minHeapResortNode(dist_heap, heap_node);
	}
}

int dijkstra(track_node *track_nodes, track_node *src,
              track_node *dest, track_node **route, int *dist) {
	if (src == NULL) {
		assert(0, "dijkstra get null src");
		return TRACK_MAX;
	} else if (dest == NULL) {
		assert(0, "dijkstra get null dest");
		return TRACK_MAX;
	}

	int i;
	track_node *previous[TRACK_MAX];
	memset(previous, (int) NULL, sizeof(track_node *) * TRACK_MAX);

	Heap dist_heap;
	HeapNode *heap_data[TRACK_MAX];
	HeapNode heap_nodes[TRACK_MAX];

	// Initialize the heap with -1 dist and landmark as datum
	heapInitial(&dist_heap, heap_data, TRACK_MAX);
	for(i = 0; i < TRACK_MAX; i++) {
		heap_nodes[i].key =  -1;
		heap_nodes[i].index = -1;
		heap_nodes[i].datum = (void *)&(track_nodes[i]);
	}

	// Source's distance is 0, and insert into the min heap
	heap_nodes[src->index].key = 0;
	minHeapInsert(&dist_heap, &(heap_nodes[src->index]));

	HeapNode *u_node = NULL;
	int u_dist = -1;
	track_node *u = NULL;
	track_node *neighbour = NULL;
	int alter_dist = -1;

	while(dist_heap.heapsize > 0) {
		// Pop the smallest distance vertex. Since node with infinit (-1) dist
		// has not been inserted into the heap, no need to check distance
		u_node = minHeapPop(&dist_heap);
		u_dist = u_node->key;
		u = u_node->datum;

		// If find destination
		if(u == dest || u == dest->reverse) break;

		switch(u->type) {
			case NODE_ENTER:
			case NODE_SENSOR:
				alter_dist = u_dist + u->edge[DIR_AHEAD].dist;
				neighbour = u->edge[DIR_AHEAD].dest;
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			case NODE_MERGE:
				alter_dist = u_dist + u->edge[DIR_AHEAD].dist;
				neighbour = u->edge[DIR_AHEAD].dest;
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				// alter_dist = u_dist + u->reverse->edge[DIR_STRAIGHT].dist + 400;
				// neighbour = u->reverse->edge[DIR_STRAIGHT].dest;
				// dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				// alter_dist = u_dist + u->reverse->edge[DIR_CURVED].dist + 400;
				// neighbour = u->reverse->edge[DIR_CURVED].dest;
				// dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			case NODE_BRANCH:
				alter_dist = u_dist + u->edge[DIR_STRAIGHT].dist;
				neighbour = u->edge[DIR_STRAIGHT].dest;
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				alter_dist = u_dist + u->edge[DIR_CURVED].dist;
				neighbour = u->edge[DIR_CURVED].dest;
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			default:
				break;
		}
	}

	// assert(u == dest, "Dijkstra failed to find the destination");
	if (u != dest && u != dest->reverse) {
		*dist = 0x7fffffff;
		return TRACK_MAX;
	}

	*dist = u_dist;

	for(i = TRACK_MAX - 1; previous[u->index] != NULL; i--) {
		route[i] = u;
		u = previous[u->index];
	}
	route[i] = src;
	return i;
}

void changeNextSW(track_node **route, int check_point, char *switch_table, int com1_tid, int com2_tid, int threshold) {
	// char cmd[2];
	int distance = 0;
	// check_point += 1;
	for (; check_point >= 0 && check_point < TRACK_MAX && distance < 1050; check_point++) {
		if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
					switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_STR;
					CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_STR, com1_tid, com2_tid);
				}
				distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
			} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
					switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_CUR;
					CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_CUR, com1_tid, com2_tid);
				}
				distance += route[check_point]->edge[DIR_CURVED].dist;
			} else {
				assert(0, "fail to change switch");
			}
		} else if (route[check_point]->type == NODE_MERGE && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] != route[check_point]->edge[DIR_AHEAD].dest) {
				if (route[check_point]->reverse->edge[DIR_STRAIGHT].dest == route[check_point + 1]) {
					if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
						switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_STR;
						CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_STR, com1_tid, com2_tid);
					}
					distance += (route[check_point]->reverse->edge)[DIR_STRAIGHT].dist;
				} else if (route[check_point]->reverse->edge[DIR_CURVED].dest == route[check_point + 1]) {
					if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
						switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_CUR;
						CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_CUR, com1_tid, com2_tid);
					}
					distance += (route[check_point]->reverse->edge)[DIR_CURVED].dist;
				} else {
					assert(0, "fail to change switch (1)");
				}
				if (route[check_point]->edge[DIR_AHEAD].dest->type == NODE_MERGE && route[check_point]->edge[DIR_AHEAD].dist < 240) {
					if (route[check_point]->edge[DIR_AHEAD].dest->reverse->edge[DIR_STRAIGHT].dest == route[check_point]->reverse) {
						if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_STR) {
							switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] = SWITCH_STR;
							CreateWithArgs(2, switchChanger, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_STR, com1_tid, com2_tid);
						}
					} else {
						if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_CUR) {
							switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] = SWITCH_CUR;
							CreateWithArgs(2, switchChanger, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_CUR, com1_tid, com2_tid);
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

int find_reverse_node(TrainData *train_data, TrainGlobal *train_global, int check_point) {
	track_node **route = train_data->route;
	int tmp = check_point;
	for (; check_point >= 0 && check_point < TRACK_MAX - 1; check_point++) {
		if (route[check_point]->type == NODE_MERGE &&
			route[check_point]->edge[DIR_AHEAD].dest != route[check_point + 1]) {
			break;
		}
	}
	if (check_point >= 0 && check_point != TRACK_MAX - 1) {
		// find the reverse node and offset
		int dist = 0;
		track_node *checked_node = route[check_point];
		train_data->merge_node = route[check_point];
		if (route[check_point]->reverse->edge[DIR_STRAIGHT].dest == route[check_point + 1]) {
			train_data->merge_state = SWITCH_STR;
		} else {
			train_data->merge_state = SWITCH_CUR;
		}
		int continue_loop = 1;
		int direction;
		while (continue_loop) {
			switch(checked_node->type) {
				case NODE_ENTER:
				case NODE_SENSOR:
				case NODE_MERGE:
					if (dist + checked_node->edge[DIR_AHEAD].dist > train_data->margin){
						continue_loop = 0;
						break;
					} else {
						dist += checked_node->edge[DIR_AHEAD].dist;
						checked_node = checked_node->edge[DIR_AHEAD].dest;
						break;
					}
				case NODE_BRANCH:
					direction = train_global->switch_table[switchIdToIndex(checked_node->num)] - 33;
					if (dist + checked_node->edge[direction].dist > train_data->margin){
						continue_loop = 0;
						break;
					} else {
						dist += checked_node->edge[direction].dist;
						checked_node = checked_node->edge[direction].dest;
						break;
					}
				default:
					IDEBUG(DB_ROUTE, 4, 54, 2 + train_data->index * 40, "exit: %s, start: %s ", checked_node->name, route[tmp]->name);
					assert(0, "find_reverse_node checked an exit node");
			}
		}
		train_data->reverse_node_offset = (train_data->margin - dist) << DIST_SHIFT;
		train_data->reverse_node = checked_node;
		return 1;
	} else {
		return 0;
	}
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

// void reserveInReversing(TrainGlobal *train_global, TrainData *train_data) {
	// bwprintf(COM2, "Reverse %d + %dmm\n", train_data->landmark->index, (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
	// IDEBUG(DB_RESERVE, 4, ROW_DEBUG_1 + 7, 40 * train_data->index + 2, "Reverse %d + %dmm", train_data->landmark->index, (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
	// CreateWithArgs(RESERVER_PRIORITY, trackReserver, (int)train_global,
				   // (int)train_data, train_data->landmark->index,
				   // (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
	// train_data->waiting_for_reserver = TRUE;
	// train_data->last_reservation_time = train_data->timer;
	// train_data->dist_since_last_rs = 0;
// }

// return route start
int findRoute(track_node *track_nodes, TrainData *train_data, track_node *dest, track_node **route, int *need_reverse) {
	train_data->on_route = TRUE;
	assert(dest != NULL, "findroute dest is null");
	int forward_dist = 0;
	int backward_dist = 0;
	int forward_start = TRACK_MAX;
	int backward_start = TRACK_MAX;
	// iprintf(4, 30, "\e[s\e[19;2HfindRoute\e[u");
	track_node *forward_route[TRACK_MAX];
	track_node *backward_route[TRACK_MAX];
	if (train_data->landmark->type != NODE_BRANCH && train_data->landmark->type != NODE_MERGE) {
		forward_start = dijkstra(track_nodes, train_data->landmark, dest, forward_route, &forward_dist);
		backward_start = dijkstra(track_nodes, train_data->landmark->reverse, dest, backward_route, &backward_dist);
	} else {
		forward_start = dijkstra(track_nodes, train_data->predict_dest, dest, forward_route, &forward_dist);
		backward_start = dijkstra(track_nodes, train_data->predict_dest->reverse, dest, backward_route, &backward_dist);
	}

	// iprintf(4, 40, "\e[s\e[20;2Hf: %d, fs: %d, b: %d, bs: %d\e[u", forward_dist - (find_stop_dist(train_data) >> DIST_SHIFT), forward_start, backward_dist + (find_stop_dist(train_data) >> DIST_SHIFT), backward_start);
	if (forward_dist - 2 * (find_stop_dist(train_data) >> DIST_SHIFT) - 2 * (train_data->ahead_lm >> DIST_SHIFT) < backward_dist) {
		*need_reverse = 0;
		int i;
		for (i = forward_start; i < TRACK_MAX; i++) {
			route[i] = forward_route[i];
		}
		return forward_start;
	} else {
		*need_reverse = 1;
		int i;
		for (i = backward_start; i < TRACK_MAX; i++) {
			route[i] = backward_route[i];
		}
		return backward_start;
	}
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
	
	train_data->sensor_for_reserve = NULL;
	setTrainSpeed(train_data->id, TRAIN_REVERSE, train_global->com1_tid);

	if (train_data->direction == FORWARD) {
		train_data->direction = BACKWARD;
	} else {
		train_data->direction = FORWARD;
	}
	if (train_data->landmark->type != NODE_EXIT) {
		train_data->landmark = train_data->predict_dest->reverse;
		train_data->ahead_lm = train_data->forward_distance - train_data->ahead_lm;
		train_data->forward_distance = (getNextNodeDist(train_data->landmark, switch_table, &direction) << DIST_SHIFT);
		train_data->predict_dest = train_data->landmark->edge[direction].dest;
	} else {
		train_data->landmark = train_data->landmark->reverse;
		train_data->forward_distance = (getNextNodeDist(train_data->landmark, switch_table, &direction) << DIST_SHIFT);
		train_data->predict_dest = train_data->landmark->edge[direction].dest;
	}

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
		train_data->landmark = train_data->predict_dest;
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
		train_data->acceleration_step = train_data->velocity * 5 / train_data->velocities[14];
		assert(train_data->acceleration_step < 5, "acceleration step exceeds");
		train_data->acceleration = train_data->accelerations[train_data->acceleration_step];
		train_data->acceleration_alarm = getTimerValue(TIMER3_BASE) - train_data->acceleration_time;
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

void updateTrainStatus(TrainGlobal *train_global, TrainData *train_data, int *v_to_0) {
	// handle acceleration
	if (train_data->acceleration_alarm > train_data->timer) {
		switch(train_data->acceleration_step) {
			case 0:
			case 1:
			case 2:
			case 3:
				train_data->acceleration_step++;
				train_data->acceleration_alarm = getTimerValue(TIMER3_BASE) - train_data->acceleration_time;
				train_data->acceleration = train_data->accelerations[train_data->acceleration_step];
				break;
			case 4:
				train_data->acceleration_alarm = 0;
				train_data->acceleration_step = -1;
				break;
			default:
				assert(0, "invalid acceleration_step");
		}
	}
	
	// handle velocity
	*v_to_0 = 0;
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
				*v_to_0 = 1;
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
					changeNextSW(train_data->route, train_data->check_point, train_global->switch_table, train_global->com1_tid, train_global->com2_tid, find_stop_dist(train_data) >> DIST_SHIFT);
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
				uiprintf(train_global->com2_tid, 51, 2, "train %d is trapped!", train_data->id);
				// todo
			}
		}
	}
}

void updateReservation(TrainGlobal *train_global, TrainData *train_data) {
	if (train_data->last_reservation_time - train_data->timer > 2000) {
		if (train_data->waiting_for_reserver) {
			changeSpeed(train_global, train_data, 0);
			train_data->acceleration = train_data->deceleration;
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
				IDEBUG(DB_ROUTE, 4, ROW_DEBUG_2 + 5, COLUMN_FIRST, "stop, %s, %d v: %d  sd: %d          ", train_data->landmark->name, train_data->ahead_lm >> 18, train_data->velocity, find_stop_dist(train_data) >> 18);
			}
		} else if (train_data->action == Goto_Merge) {
			train_data->stop_type = destCheck(train_global, train_data, 
										find_stop_dist(train_data), train_data->reverse_node, train_data->reverse_node_offset);
			if (train_data->stop_type == Entering_Merge) {
				changeSpeed(train_global, train_data, 0);
			}
		} else if (train_data->action == Goto_Dest) {
			train_data->stop_type = destCheck(train_global, train_data, 
										find_stop_dist(train_data), train_data->stop_node, 0);
			if (train_data->stop_type == Entering_Dest) {
				changeSpeed(train_global, train_data, 0);
			}
		}
	}
}

void changeState(TrainGlobal *train_global, TrainData *train_data, int v_to_0) {
	int need_reverse;
	int tmp_dist;
	int result;
	if (v_to_0) {
		uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_3, "%d ", train_data->stop_type);
		switch(train_data->stop_type) {
			case Entering_Exit:
				if (train_data->stop_node != NULL) {
					reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
					train_data->route_start = findRoute(train_global->track_nodes, train_data, train_data->stop_node, train_data->route, &need_reverse);
					train_data->stop_node = train_data->route[TRACK_MAX - 1];
					train_data->check_point = train_data->route_start;
					assert(!need_reverse, "cannot reverse to exit node");
					// reserveInReversing(train_global, train_data);
					train_data->stop_type = RB_go;
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
				if (train_global->switch_table[switchIdToIndex(train_data->merge_node->num)] != train_data->merge_state) {
					train_global->switch_table[switchIdToIndex(train_data->merge_node->num)] = train_data->merge_state;
					CreateWithArgs(2, switchChanger, train_data->merge_node->num, train_data->merge_state, train_global->com1_tid, train_global->com2_tid);
				}
					
				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
				// reserveInReversing(train_global, train_data);
				train_data->reverse_node = NULL;
				train_data->reverse_node_offset = 0;
				train_data->merge_node = NULL;
				train_data->route_start = findRoute(train_global->track_nodes, train_data, train_data->stop_node, train_data->route, &need_reverse);
				train_data->stop_node = train_data->route[TRACK_MAX - 1];
				train_data->check_point = train_data->route_start;
				if (need_reverse) {
					IDEBUG(DB_ROUTE, 4, ROW_DEBUG_2 + 3, COLUMN_FIRST, "error! cur: %s, dest: %s", train_data->landmark->name, train_data->stop_node->name);
				}
				uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				if (find_reverse_node(train_data, train_global, train_data->check_point)) {
					train_data->action = Goto_Merge;
					train_data->route_start = dijkstra(train_global->track_nodes, train_data->landmark, train_data->reverse_node, train_data->route, &tmp_dist);
					assert(train_data->route_start != TRACK_MAX, "route_start == TRACK_MAX");
					train_data->check_point = train_data->route_start;
				} else {
					train_data->action = Goto_Dest;
				}
				train_data->stop_type = RB_go;
				break;
				
			case Reversing:
				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
				// reserveInReversing(train_global, train_data);
				if (train_data->action != Free_Run) {
					train_data->route_start = findRoute(train_global->track_nodes, train_data, train_data->stop_node, train_data->route, &need_reverse);
					if (need_reverse) {
						IDEBUG(DB_ROUTE, train_global->com2_tid, ROW_DEBUG_2 + 4, COLUMN_FIRST, "shouldn't reverse: %s   ", train_data->landmark->name);
					}
					train_data->stop_node = train_data->route[TRACK_MAX - 1];
					train_data->check_point = train_data->route_start;
					if (find_reverse_node(train_data, train_global, train_data->check_point)) {
						train_data->action = Goto_Merge;
						train_data->route_start = dijkstra(train_global->track_nodes, train_data->landmark, train_data->reverse_node, train_data->route, &tmp_dist);
						train_data->check_point = train_data->route_start;
					} else {
						train_data->action = Goto_Dest;
					}
					uiprintf(train_global->com2_tid, ROW_TRAIN + train_data->index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				}
				train_data->stop_type = RB_go;
				break;
			
			case Stopped:
				train_data->stop_type = None;
				break;
			
			case RB_slowing:
				reverseTrainAndLandmark(train_global, train_data, train_global->switch_table);
				result = gotoNode(train_data->landmark, train_data->last_receive_sensor->reverse, train_global, 8);
				if (!result) {
					IDEBUG(DB_ROUTE, 4, 57, 40 * train_data->index + 2, "%s -> %s ", train_data->landmark->name, train_data->last_receive_sensor->reverse->name);
				}
				
				train_data->action = RB_last_ss;
				train_data->stop_type = RB_go;
				// reserveInRecovery(train_global, train_data);
				break;
				
			case RB_go:
				assert(0, "should stop in RB_last_ss or RB_go");
				break;
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
	int tmp_dist;
	TrainMsg msg;
	char str_buf[1024];
	int need_reverse;

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
			
			int v_to_0;
			updateTrainStatus(train_global, train_data, &v_to_0);
			updateReservation(train_global, train_data);
			stopChecking(train_global, train_data);
			changeState(train_global, train_data, v_to_0);
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
				if (train_data->action == RB_last_ss) {
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
						changeNextSW(train_data->route, train_data->check_point, train_global->switch_table, com1_tid, com2_tid, find_stop_dist(train_data) >> DIST_SHIFT);
					}
				}
				
				if (need_recalculate) {
					train_data->route_start = findRoute(track_nodes, train_data, train_data->stop_node, train_data->route, &need_reverse);
					train_data->check_point = train_data->route_start;
					if (need_reverse) {
						changeSpeed(train_global, train_data, 0);
						train_data->check_point = -1;
						train_data->action = Off_Route;
						train_data->stop_type = Reversing;
					} else {
						train_data->stop_node = train_data->route[TRACK_MAX - 1];
						IDEBUG(DB_ROUTE, com2_tid, ROW_DEBUG_2 + 5, COLUMN_FIRST, "%s (R) ", train_data->stop_node->name);

						if (find_reverse_node(train_data, train_global, train_data->check_point)) {
							train_data->action = Goto_Merge;
							train_data->route_start = dijkstra(track_nodes, train_data->landmark, train_data->reverse_node, train_data->route, &tmp_dist);
							assert(train_data->route_start != TRACK_MAX, "route_start == TRACK_MAX");
							train_data->check_point = train_data->route_start;
						} else {
							train_data->action = Goto_Dest;
						}
						changeNextSW(train_data->route, train_data->check_point, train_global->switch_table, com1_tid, com2_tid, find_stop_dist(train_data) >> DIST_SHIFT);
					}
				}

				break;
			case CMD_GOTO:
				Reply(tid, NULL, 0);

				train_data->route_start = findRoute(track_nodes, train_data, &(track_nodes[msg.location_msg.value]), train_data->route, &need_reverse);
				train_data->stop_node = &(track_nodes[msg.location_msg.value]);
				if (need_reverse) {
					changeSpeed(train_global, train_data, 0);
					train_data->check_point = -1;
					train_data->action = Off_Route;
					train_data->stop_type = Reversing;
				} else {
					train_data->stop_node = train_data->route[TRACK_MAX - 1];
					train_data->check_point = train_data->route_start;
					// int prt_cnter = route_start;
					// buf_cursor += sprintf(buf_cursor, "\e[s\e[%d;%dH", ROW_DEBUG_2 + 10, COLUMN_FIRST);
					// for(; prt_cnter < TRACK_MAX; prt_cnter++) {
						// buf_cursor += sprintf(buf_cursor, "%s -> ", route[prt_cnter]->name);
					// }
					// buf_cursor += sprintf(buf_cursor, "\t\t\t\t\t\t\t\t\t\n\e[u");
					// Puts(com2_tid, str_buf, 0);
					// str_buf[0] = '\0';
					if (find_reverse_node(train_data, train_global, train_data->check_point)) {
						train_data->action = Goto_Merge;
						train_data->route_start = dijkstra(track_nodes, train_data->landmark, train_data->reverse_node, train_data->route, &tmp_dist);
						assert(train_data->route_start != TRACK_MAX, "route_start == TRACK_MAX");
						train_data->check_point = train_data->route_start;
					} else {
						train_data->action = Goto_Dest;
					}

					changeNextSW(train_data->route, train_data->check_point, train_global->switch_table, com1_tid, com2_tid, find_stop_dist(train_data) >> DIST_SHIFT);
					// if (check_point != -1) {
						// sprintf(str_buf, "branch change: %s\n", route[check_point]->name);
						// Puts(com2_tid, str_buf, 0);
					// }
					uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", train_data->stop_node->name);
				}
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
					IDEBUG(DB_ROUTE, 4, 54, 5 + train_data->index * 40, "__%s", train_data->direction == FORWARD ? "F" : "B");
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
			default:
				sprintf(str_buf, "Driver got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
		buf_cursor = str_buf;
	}
}