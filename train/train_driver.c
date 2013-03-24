#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

#define DELAY_REVERSE 200

typedef enum {
	Free_Run,
	Goto_Merge,
	Goto_Dest
} Action;

typedef enum {
	Entering_None = 0,
	Entering_Exit,
	Entering_Dest,
	Entering_Merge,
	Stopped,
	Reversing,
	Reserve_Blocked
} StopType;


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
	TrainMsgType reply;
	ReservationMsg msg;
	msg.type = TRACK_RESERVE;
	msg.train_data = train_data;
	msg.landmark_id = landmark_id;
	msg.distance = distance;
	driver_tid = MyParentTid();

	// Send reservation request to the center
	result = Send(center_tid, (char *)(&msg), sizeof(ReservationMsg), (char *)(&reply), sizeof(TrainMsgType));
	assert(result > 0, "Reserver fail to receive reply from center");

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

int calcDistance(track_node *src, track_node *dest, int depth, int distance) {
	// If found return the distance
	if(src == dest) {
		return distance;
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
			return calcDistance(src->edge[DIR_AHEAD].dest, dest, depth - 1, distance + src->edge[DIR_AHEAD].dist);
		case NODE_BRANCH:
			straight = calcDistance(src->edge[DIR_STRAIGHT].dest, dest, depth - 1, distance + src->edge[DIR_STRAIGHT].dist);
			if(straight > 0) return straight;
			curved = calcDistance(src->edge[DIR_CURVED].dest, dest, depth - 1, distance + src->edge[DIR_CURVED].dist);
			return curved;
		default:
			return -1;
	}

	return -1;
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

StopType stopCheck(track_node *cur_node, track_node **exit_node, int ahead, char *switch_table, int stop_distance, track_node *stop_node, int offset, track_node **route, int check_point, int com2_tid) {

	int distance = 0;
	int direction;
	track_node *checked_node = cur_node;
	int hit_exit = 0;
	if (stop_node != NULL && offset == -1) {
		IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 1, COLUMN_FIRST, "cur: %s  ", route[check_point]->name);
		for (; distance < (stop_distance + ahead) && !hit_exit && check_point < TRACK_MAX; check_point++) {
			if (stop_node == route[check_point]) {
				return Entering_Dest;
			}
			if (route[check_point]->type == NODE_EXIT) {
				*exit_node = route[check_point];
				return Entering_Exit;
			}
			if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
				if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
					distance += ((route[check_point]->edge[DIR_STRAIGHT].dist) << DIST_SHIFT);
				} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
					distance += ((route[check_point]->edge[DIR_CURVED].dist) << DIST_SHIFT);
				} else {
					bwprintf(COM2, "%s, %s", route[check_point]->name, route[check_point + 1]->name);
					assert(0, "die ah sitra branch cannot find a dest");
				}
			} else {
				distance += (route[check_point]->edge[DIR_AHEAD].dist <<  DIST_SHIFT);
			}
		}
	} else if (stop_node != NULL && offset != -1) {
		while (distance + offset < (stop_distance + ahead) && !hit_exit) {
			if (stop_node == checked_node) {
				return Entering_Merge;
			}
			switch(checked_node->type) {
				case NODE_SENSOR:
				case NODE_MERGE:
				case NODE_ENTER:
					distance += (checked_node->edge[DIR_AHEAD].dist << DIST_SHIFT);
					checked_node = checked_node->edge[DIR_AHEAD].dest;
					break;
				case NODE_BRANCH:
					for (; check_point < TRACK_MAX - 1; check_point++) {
						if (route[check_point] == checked_node) {
							if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
								distance += ((route[check_point]->edge[DIR_STRAIGHT].dist) << DIST_SHIFT);
								checked_node = checked_node->edge[DIR_STRAIGHT].dest;
							} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
								distance += ((route[check_point]->edge[DIR_CURVED].dist) << DIST_SHIFT);
								checked_node = checked_node->edge[DIR_CURVED].dest;
							} else {
								assert(0, "die ah sitra branch cannot find a dest(1)");
							}
							break;
						}
					}
					if (check_point == TRACK_MAX - 1) {
						direction = switch_table[switchIdToIndex(checked_node->num)] - 33;
						distance += (checked_node->edge[direction].dist << DIST_SHIFT);
						checked_node = checked_node->edge[direction].dest;
					}
					break;
				case NODE_EXIT:
					*exit_node = checked_node;
					return Entering_Exit;
					break;
				default:
					bwprintf(COM2, "node type: %d\n", checked_node->type);
					assert(0, "stopCheck function cannot find a valid node type(1)");
					break;
			}
		}
	} else {
		while (distance < (stop_distance + ahead) && !hit_exit) {
			switch(checked_node->type) {
				case NODE_SENSOR:
				case NODE_MERGE:
				case NODE_ENTER:
					distance += (checked_node->edge[DIR_AHEAD].dist << DIST_SHIFT);
					checked_node = checked_node->edge[DIR_AHEAD].dest;
					break;
				case NODE_BRANCH:
					direction = switch_table[switchIdToIndex(checked_node->num)] - 33;
					distance += (checked_node->edge[direction].dist << DIST_SHIFT);
					checked_node = checked_node->edge[direction].dest;
					break;
				case NODE_EXIT:
					*exit_node = checked_node;
					return Entering_Exit;
					break;
				default:
					bwprintf(COM2, "node type: %d\n", checked_node->type);
					assert(0, "stopCheck function cannot find a valid node type(1)");
					break;
			}
		}
	}

	// iprintf(com2_tid, 70, "\e[s\e[31;2Hcheck_point: %d  %s \e[u", check_point, route[check_point]->name);
	// if (hit_exit) {
		// if ((stop_distance + ahead) - distance > (20 <<  DIST_SHIFT)) {
			// assert(0, "WARNING!   TRAIN IS GONNA CRASH!");
		// }
		// iprintf(com2_tid, 70, "\e[s\e[30;2Hdistance: %d, ahead+stop_distance: %d \e[u" , distance, ahead+stop_distance);
		// if (stop_node != NULL )
		// iprintf(com2_tid, 70, "\e[s\e[30;2Hdistance: %d, ahead+stop_distance: %d, %s\e[u", distance, ahead+stop_distance, route[ck]->name);
		// return hit_exit;
	// }
	return Entering_None;
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
	if (src == NULL || dest == NULL) {
		assert(0, "dijkstra get null src or dest");
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

void changeNextSW(track_node **route, int check_point, char *switch_table, int com1_tid, int com2_tid) {
	// char cmd[2];
	int distance = 0;
	// check_point += 1;
	for (; check_point < TRACK_MAX && distance < 1200; check_point++) {
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

int find_reverse_node(track_node **route, int check_point, char *switch_table, int *offset, track_node **reverse_node, int margin) {
	for (; check_point < TRACK_MAX - 1; check_point++) {
		if (route[check_point]->type == NODE_MERGE &&
			route[check_point]->edge[DIR_AHEAD].dest != route[check_point + 1]) {
			break;
		}
	}
	if (check_point != TRACK_MAX - 1) {
		// find the reverse node and offset
		int dist = 0;
		track_node *checked_node = route[check_point];
		int continue_loop = 1;
		int direction;
		while (continue_loop) {
			switch(checked_node->type) {
				case NODE_ENTER:
				case NODE_SENSOR:
				case NODE_MERGE:
					if (dist + checked_node->edge[DIR_AHEAD].dist > margin){
						continue_loop = 0;
						break;
					} else {
						dist += checked_node->edge[DIR_AHEAD].dist;
						checked_node = checked_node->edge[DIR_AHEAD].dest;
						break;
					}
				case NODE_BRANCH:
					direction = switch_table[switchIdToIndex(checked_node->num)] - 33;
					if (dist + checked_node->edge[direction].dist > margin){
						continue_loop = 0;
						break;
					} else {
						dist += checked_node->edge[direction].dist;
						checked_node = checked_node->edge[direction].dest;
						break;
					}
				default:
					assert(0, "find_reverse_node checked an exit node");
			}
		}
		*offset = (margin - dist) << DIST_SHIFT;
		IDEBUG(DB_ROUTE, 4, ROW_DEBUG, COLUMN_FIRST, "reverse node: %s, offset: %d       ", checked_node->name, (*offset) >> 18);
		*reverse_node = checked_node;
		return 1;
	} else {
		return 0;
	}
}

// return route start
int findRoute(track_node *track_nodes, TrainData *train_data, track_node *dest, track_node **route, int *need_reverse) {
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
	if (check_point + 1 < TRACK_MAX) {
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

//update current landmark
void updateCurrentLandmark(TrainGlobal *train_global, TrainData *train_data, track_node *sensor_node, char *switch_table, int com2_tid, int reverse) {
	int train_index = train_data->index;
	int direction;
	if (!reverse) {
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
				// iprintf(com2_tid, 30, "\e[s\e[%d;%dHlocation -1 %s\e[u", 36, 2, train_data->landmark->name);
			}
		} else {
			train_data->ahead_lm = 0;
			train_data->forward_distance = 0;
		}
	} else {
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
	}

	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_1, "%s  ", train_data->landmark->name);
	uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_1, "%s  ", train_data->predict_dest->name);

	// Test track reservation
	CreateWithArgs(7, trackReserver, (int)train_global, (int)train_data, train_data->landmark->index, (find_stop_dist(train_data) + train_data->ahead_lm) >> DIST_SHIFT);
}




void trainDriver(TrainGlobal *train_global, TrainData *train_data) {
	track_node *track_nodes = train_global->track_nodes;
	track_node *stop_node = NULL;
	int tid, result;
	int train_id = train_data->id;
	int train_index = train_data->index;
	int speed = 0;
	int old_speed = 0;
	int speed_before_reverse = 0;

	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;
	int delay_time;

	TrainMsg msg;
	unsigned int timer = 0; 		// 1/2000s
	unsigned int prev_timer = 0;	// 1/2000s
	char str_buf[1024];
	// int tmp_direction;

	// unsigned int position_alarm = 0;
	// unsigned int stop_alarm = 0;
	// unsigned int reverse_alarm = 0;
	// unsigned int freeze_point = 0;
	int acceleration = 0;
	int speed_change_time = 0;
	unsigned int speed_change_alarm = 0;
	int speed_change_step = 0;

	int check_point = -1; 		// for finding route
	track_node *exit_node = NULL;
	int margin = 225;

	// initialize start position
	updateCurrentLandmark(train_global, train_data,
	                      &(track_nodes[train_data->reservation_record.landmark_id]),
	                      train_global->switch_table, com2_tid, FALSE);

	char *buf_cursor = str_buf;

	// Initialize train speed
	setTrainSpeed(train_id, speed, com1_tid);

	int secretary_tid = Create(2, trainSecretary);

	// route variables
	track_node *route[TRACK_MAX];
	int route_start = TRACK_MAX;
	int reverse_node_offset = 0;
	unsigned int reverse_protection_alarm = 0;
	int reverse_protect = 0;
	// int on_route = 0;

	int cnt = 0;

	Action action = Free_Run;
	StopType stop_type = Entering_None;

	// int reversing = 0;
	track_node *reverse_node = NULL;
	// track_node *merge_node = NULL;
	// int simple_reverse = 0;


	char cmd[2];

	// iprintf(com2_tid, 10, "\e[s\e[20;2Hcom2: %d \e[u", com2_tid);

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");
		if (tid == secretary_tid) {
			Reply(tid, NULL, 0);
			timer = getTimerValue(TIMER3_BASE);

			// location update
			train_data->ahead_lm = train_data->ahead_lm + train_data->velocity * (prev_timer - timer);
			if (train_data->ahead_lm > train_data->forward_distance) {
				updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table, com2_tid, FALSE);
				if (action != Free_Run) {
					int tmp = updateCheckPoint(train_data, route, check_point, route_start);
					if (tmp != -1) {
						check_point = tmp;
						if (train_data->landmark->type == NODE_SENSOR) {
							changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
						}
					// assert(check_point != -1, "check_point is -1");
					} else {
						IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 2, COLUMN_FIRST, "off route: %s   ", train_data->landmark->name);
					}
				}
			}

			if (cnt == 8) cnt = 0;
			if (cnt == 0) {
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_STATUS,
				         COLUMN_DATA_1, "%d   ", train_data->velocity);
			}

			if (speed_change_alarm > timer) {
				switch(speed_change_step) {
					case 1:
						speed_change_step++;
						speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
						acceleration = train_data->acceleration_G2;
						break;
					case 2:
						speed_change_step++;
						speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
						acceleration = train_data->acceleration_G3;
						break;
					case 3:
						speed_change_step++;
						speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
						acceleration = train_data->acceleration_G4;
						break;
					case 4:
						speed_change_alarm = 0;
						speed_change_step = 0;
						speed_change_time = 0;
						acceleration = train_data->acceleration_G5;
						break;
					// case 1:
						// speed_change_alarm = 0;
						// speed_change_step = 0;
						// speed_change_time = 0;
						// speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 2;
						// acceleration = train_data->acceleration_high;
						// break;

					default:
						assert(0, "wrf???");
				}
			}
			if (reverse_protection_alarm > timer) {
				reverse_protection_alarm = 0;
				reverse_protect = 0;
			}

			train_data->velocity = train_data->velocity + (prev_timer - timer) * acceleration;
			cnt++;
			if (cnt == 8) {
				cnt = 0;
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_STATUS,
				         COLUMN_DATA_1, "%d   ", train_data->velocity);
			}

			if (acceleration > 0) {
				if (train_data->velocity > train_data->velocities[speed % 16]) {
					train_data->velocity = train_data->velocities[speed % 16];
					acceleration = 0;
					speed_change_alarm = 0;
					speed_change_step = 0;
					speed_change_time = 0;
				}

			} else if (acceleration < 0) {
				if (train_data->velocity < train_data->velocities[speed % 16]) {
					train_data->velocity = train_data->velocities[speed % 16];
					acceleration = 0;
					if (train_data->velocity == 0) {

						switch(stop_type) {
							case Entering_Exit:
								stop_type = Entering_None;
								// updateCurrentLandmark(train_global, train_data, exit_node, train_global->switch_table, com2_tid);
								// if (stop_node == train_data->landmark) {
									// stop_node = NULL;
									// iprintf(com2_tid, 30, "\e[s\e[%d;%dH       \e[u", 15, 15);
									// action = Free_Run;
								// }
								break;
							case Entering_Dest:
								stop_type = Entering_None;
								// updateCurrentLandmark(train_global, train_data, stop_node, train_global->switch_table, com2_tid);
								stop_node = NULL;
								action = Free_Run;
								uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "    ");
								break;
							case Entering_Merge:
								stop_type = Entering_None;
								updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table, com2_tid, TRUE);
								// train_data->forward_distance = (getNextNodeDist(reverse_node, train_global->switch_table, &tmp_direction) << DIST_SHIFT);
								// train_data->landmark = reverse_node->edge[tmp_direction].dest->reverse;
								// train_data->predict_dest = reverse_node->reverse;
								// train_data->ahead_lm = train_data->forward_distance - reverse_node_offset;

								// if (train_data->landmark->type != NODE_EXIT) {
									// track_node *tmp;
									// tmp = train_data->landmark;
									// train_data->landmark = train_data->predict_dest->reverse;
									// train_data->predict_dest = tmp->reverse;
									// train_data->ahead_lm = train_data->forward_distance - train_data->ahead_lm;
								// } else {
									// train_data->landmark = train_data->landmark->reverse;
									// train_data->forward_distance = (getNextNodeDist(train_data->landmark, train_global->switch_table, &tmp_direction) << DIST_SHIFT);
									// train_data->predict_dest = train_data->landmark->edge[tmp_direction].dest;
								// }

								// iprintf(com2_tid, 30, "\e[s\e[%d;%dH%s  \e[u", 11, 20, train_data->landmark->name);
								// iprintf(com2_tid, 30, "\e[s\e[%d;%dH%s  \e[u", 12, 17, train_data->predict_dest->name);
								reverse_node = NULL;
								reverse_node_offset = 0;
								int need_reverse = 1;
								route_start = findRoute(track_nodes, train_data, stop_node, route, &need_reverse);
								stop_node = route[TRACK_MAX - 1];
								check_point = route_start;
								if (need_reverse) {
									// acceleration = train_data->deceleration;;
									// speed_before_reverse = speed;
									// speed = 0;
									// setTrainSpeed(train_id, 0, com1_tid);
									// stop_type = Reversing;
									IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 3, COLUMN_FIRST, "error! cur: %s, dest: %s", train_data->landmark->name, stop_node->name);
								}
								uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", stop_node->name);
								// check_point = route_start - 1;
								// route[check_point] = train_data->landmark;
								if (find_reverse_node(route, check_point, train_global->switch_table, &reverse_node_offset, &reverse_node, margin)) {
									action = Goto_Merge;
								} else {
									action = Goto_Dest;
								}
								cmd[0] = TRAIN_REVERSE;
								cmd[1] = train_id;
								Puts(com1_tid, cmd, 2);
								speed = speed_before_reverse;
								cmd[0] = speed;
								Puts(com1_tid, cmd, 2);
								Delay(train_data->reverse_delay);
								timer = getTimerValue(TIMER3_BASE);
								reverse_protection_alarm = timer - 1000;
								reverse_protect = 1;
								if (speed > 0) {
									speed_change_time = train_data->acceleration_time * train_data->velocities[speed%16] / train_data->velocities[14];
									speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
									acceleration = train_data->acceleration_G1;
									speed_change_step = 1;
								}
								break;
							case Reversing:
								stop_type = Entering_None;
								updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table, com2_tid, TRUE);

								// if (train_data->landmark->type != NODE_EXIT) {
									// track_node *tmp;
									// tmp = train_data->landmark;
									// train_data->landmark = train_data->predict_dest->reverse;
									// train_data->predict_dest = tmp->reverse;
									// train_data->ahead_lm = train_data->forward_distance - train_data->ahead_lm;
								// } else {
									// train_data->landmark = train_data->landmark->reverse;
									// train_data->forward_distance = (getNextNodeDist(train_data->landmark, train_global->switch_table, &tmp_direction) << DIST_SHIFT);
									// train_data->predict_dest = train_data->landmark->edge[tmp_direction].dest;
								// }
								// iprintf(com2_tid, 30, "\e[s\e[%d;%dH%s  \e[u", 11, 20, train_data->landmark->name);
								// iprintf(com2_tid, 30, "\e[s\e[%d;%dH%s  \e[u", 12, 17, train_data->predict_dest->name);
								cmd[0] = TRAIN_REVERSE;
								cmd[1] = train_id;
								Puts(com1_tid, cmd, 2);
								speed = speed_before_reverse;
								cmd[0] = speed;
								Puts(com1_tid, cmd, 2);
								Delay(train_data->reverse_delay);
								timer = getTimerValue(TIMER3_BASE);
								reverse_protection_alarm = timer - 1000;
								reverse_protect = 1;
								if (speed > 0) {
									speed_change_time = train_data->acceleration_time * train_data->velocities[speed%16] / train_data->velocities[14];
									speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
									acceleration = train_data->acceleration_G1;
									speed_change_step = 1;
								}
								if (stop_node != NULL) {
									int need_reverse = 1;
									route_start = findRoute(track_nodes, train_data, stop_node, route, &need_reverse);
									if (need_reverse) {
										IDEBUG(DB_ROUTE, com2_tid, ROW_DEBUG + 4, COLUMN_FIRST, "shouldn't reverse: %s   ", train_data->landmark->name);
										acceleration = train_data->deceleration;;
										speed_change_alarm = 0;
										speed_change_step = 0;
										speed_change_time = 0;
										speed_before_reverse = speed;
										speed = 0;
										setTrainSpeed(train_id, 0, com1_tid);
										stop_type = Reversing;
									}
									assert(route_start != TRACK_MAX, "route_start == TRACK_MAX");
									stop_node = route[TRACK_MAX - 1];
									check_point = route_start;
									if (find_reverse_node(route, check_point, train_global->switch_table, &reverse_node_offset, &reverse_node, margin)) {
										action = Goto_Merge;
									} else {
										action = Goto_Dest;
									}
							        uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", stop_node->name);
								}
								break;
							case Stopped:
								stop_type = Entering_None;
								break;
							case Reserve_Blocked:
								stop_type = Entering_None;
								updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table, com2_tid, TRUE);
								route_start = dijkstra(track_nodes, train_data->landmark, stop_node, forward_route, &forward_dist);
								if (route_start == TRACK_MAX) {
									assert(0, "cannot find route");
									stop_node = NULL;
								} else {
									check_point = route_start;
									changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
									stop_node = route[TRACK_MAX - 1];
									uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", stop_node->name);
									cmd[0] = TRAIN_REVERSE;
									cmd[1] = train_id;
									Puts(com1_tid, cmd, 2);
									speed = speed_before_reverse;
									cmd[0] = speed;
									Puts(com1_tid, cmd, 2);
									Delay(train_data->reverse_delay);
									timer = getTimerValue(TIMER3_BASE);
									reverse_protection_alarm = timer - 1000;
									reverse_protect = 1;
									if (speed > 0) {
										speed_change_time = train_data->acceleration_time * train_data->velocities[speed%16] / train_data->velocities[14];
										speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
										acceleration = train_data->acceleration_G1;
										speed_change_step = 1;
									}
								}
								break;
							default:
								assert(0, "missing stop type");
						}
					}
				}
			}

			// stop checking
			if (speed % 16 != 0) {
				if (action == Free_Run || action == Goto_Dest) {
					stop_type = stopCheck(train_data->landmark, &exit_node, train_data->ahead_lm, train_global->switch_table,
					find_stop_dist(train_data), stop_node, -1, route, check_point, com2_tid);
					if (stop_type == Entering_Exit || stop_type == Entering_Dest) {
						setTrainSpeed(train_id, 0, com1_tid);
						speed = 0;
						acceleration = train_data->deceleration;;
						speed_change_alarm = 0;
						speed_change_step = 0;
						speed_change_time = 0;
						IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 5, COLUMN_FIRST, "stop, %s, %d v: %d  sd: %d          ", train_data->landmark->name, train_data->ahead_lm >> 18, train_data->velocity, find_stop_dist(train_data) >> 18);
					}

				} else if (action == Goto_Merge) {
					stop_type = stopCheck(train_data->landmark, &exit_node, train_data->ahead_lm, train_global->switch_table,
					find_stop_dist(train_data), reverse_node, reverse_node_offset, route, check_point, com2_tid);
					if (stop_type == Entering_Exit) {
						setTrainSpeed(train_id, 0, com1_tid);
						speed = 0;
						acceleration = train_data->deceleration;;
						speed_change_alarm = 0;
						speed_change_step = 0;
						speed_change_time = 0;
						IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 5, COLUMN_FIRST, "stop, %s, %d v: %d  sd: %d          ", train_data->landmark->name, train_data->ahead_lm >> 18, train_data->velocity, find_stop_dist(train_data) >> 18);
					}
					if (stop_type == Entering_Merge) {
						setTrainSpeed(train_id, 0, com1_tid);
						speed_before_reverse = speed;
						speed = 0;
						acceleration = train_data->deceleration;;
						speed_change_alarm = 0;
						speed_change_step = 0;
						speed_change_time = 0;
						IDEBUG(DB_ROUTE, 4, ROW_DEBUG + 5, COLUMN_FIRST, "stop, %s, %d v: %d  sd: %d          ", train_data->landmark->name, train_data->ahead_lm >> 18, train_data->velocity, find_stop_dist(train_data) >> 18);
					}
				} else {
					assert(0, "wtf!");
				}
			}

			prev_timer = timer;

			if (!cnt) {
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_CURRENT, COLUMN_DATA_2, "%d  ", train_data->ahead_lm >> DIST_SHIFT);
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_NEXT, COLUMN_DATA_2, "%d  ", (train_data->forward_distance - train_data->ahead_lm) >> DIST_SHIFT);
			}
			continue;
		}
		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);

				old_speed = speed;
				speed = msg.cmd_msg.value;

				if (speed % 16 == 0) {
					stop_type = Stopped;
				}

				if (speed % 16 > old_speed % 16) {
					speed_change_time = train_data->acceleration_time * (train_data->velocities[speed%16] - train_data->velocities[old_speed%16]) / train_data->velocities[14];
					speed_change_alarm = getTimerValue(TIMER3_BASE) - speed_change_time / 5;
					acceleration = train_data->acceleration_G1;
					speed_change_step = 1;
				} else if (speed % 16 < old_speed % 16) {
					acceleration = train_data->deceleration;;
					speed_change_alarm = 0;
					speed_change_step = 0;
					speed_change_time = 0;
				}

				setTrainSpeed(train_id, speed, com1_tid);
				break;

			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				acceleration = train_data->deceleration;;
				speed_change_alarm = 0;
				speed_change_step = 0;
				speed_change_time = 0;
				speed_before_reverse = speed;
				speed = 0;
				stop_type = Reversing;
				setTrainSpeed(train_id, 0, com1_tid);
				break;

			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				if (train_data->landmark != &(track_nodes[msg.location_msg.id])) {
					// inaccuracy print
					uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_STATUS, COLUMN_DATA_2, "-%d  ", (train_data->forward_distance - train_data->ahead_lm) >> DIST_SHIFT);

					updateCurrentLandmark(train_global, train_data, &(track_nodes[msg.location_msg.id]), train_global->switch_table, com2_tid, FALSE);

					if (stop_type == Entering_None && action != Free_Run && !reverse_protect) {
						check_point = updateCheckPoint(train_data, route, check_point, route_start);
						if (check_point != -1) {
							// changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
						} else {
							// recalculate route
							int need_reverse;
							route_start = findRoute(track_nodes, train_data, stop_node, route, &need_reverse);
							if (need_reverse) {
								acceleration = train_data->deceleration;;
								speed_change_alarm = 0;
								speed_change_step = 0;
								speed_change_time = 0;
								speed_before_reverse = speed;
								speed = 0;
								setTrainSpeed(train_id, 0, com1_tid);
								stop_type = Reversing;
							}
							assert(route_start != TRACK_MAX, "route_start == TRACK_MAX");
							check_point = route_start;
							stop_node = route[TRACK_MAX - 1];
							IDEBUG(DB_ROUTE, com2_tid, ROW_DEBUG + 5, COLUMN_FIRST, "%s (R) ", stop_node->name);

							if (find_reverse_node(route, check_point, train_global->switch_table, &reverse_node_offset, &reverse_node, margin)) {
								action = Goto_Merge;
							} else {
								action = Goto_Dest;
							}
						}
					}
					// changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
				} else {
					// inaccuracy print
					uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_STATUS, COLUMN_DATA_2, "%d", train_data->ahead_lm >> DIST_SHIFT);
					train_data->ahead_lm = 0;
				}

				if (action != Free_Run) {
					if (route[check_point] != train_data->landmark) {
						int tmp = updateCheckPoint(train_data, route, check_point, route_start);
						if (tmp != -1) {
							check_point = tmp;
							changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
						}
					} else {
						changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
					}
				}

				break;
			case CMD_GOTO:
				Reply(tid, NULL, 0);

				int need_reverse = 0;
				route_start = findRoute(track_nodes, train_data, &(track_nodes[msg.location_msg.value]), route, &need_reverse);
				if (need_reverse) {
					acceleration = train_data->deceleration;
					speed_change_alarm = 0;
					speed_change_step = 0;
					speed_change_time = 0;
					speed_before_reverse = speed;
					speed = 0;
					setTrainSpeed(train_id, 0, com1_tid);
					stop_type = Reversing;
				}

				assert(route_start != TRACK_MAX, "route_start == TRACK_MAX");
				stop_node = route[TRACK_MAX - 1];
				check_point = route_start;
				int prt_cnter = route_start;
				buf_cursor += sprintf(buf_cursor, "\e[s\e[%d;%dH", ROW_DEBUG + 10, COLUMN_FIRST);
				for(; prt_cnter < TRACK_MAX; prt_cnter++) {
					buf_cursor += sprintf(buf_cursor, "%s -> ", route[prt_cnter]->name);
				}
				buf_cursor += sprintf(buf_cursor, "\t\t\t\t\t\t\t\t\t\n\e[u");
				Puts(com2_tid, str_buf, 0);
				str_buf[0] = '\0';
				if (find_reverse_node(route, check_point, train_global->switch_table, &reverse_node_offset, &reverse_node, margin)) {
					action = Goto_Merge;
				} else {
					action = Goto_Dest;
				}

				changeNextSW(route, check_point, train_global->switch_table, com1_tid, com2_tid);
				// if (check_point != -1) {
					// sprintf(str_buf, "branch change: %s\n", route[check_point]->name);
					// Puts(com2_tid, str_buf, 0);
				// }
				uiprintf(com2_tid, ROW_TRAIN + train_index * HEIGHT_TRAIN + ROW_ROUTE, COLUMN_DATA_1, "%s  ", stop_node->name);
				break;
			case CMD_MARGIN:
				Reply(tid, NULL, 0);
				margin = msg.cmd_msg.value;
				break;
			case TRACK_RESERVE_FAIL:
				Reply(tid, NULL, 0);
				acceleration = train_data->deceleration;
				speed_change_alarm = 0;
				speed_change_step = 0;
				speed_change_time = 0;
				speed_before_reverse = speed;
				speed = 0;
				setTrainSpeed(train_id, speed, com1_tid);
				stop_type = Reserve_Blocked;
				break;
			case TRACK_RESERVE_SUCCEED:
				Reply(tid, NULL, 0);
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

