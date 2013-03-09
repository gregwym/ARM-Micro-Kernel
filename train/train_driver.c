#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

#define DELAY_REVERSE 200

void trainReverser(int train_id, int new_speed, int com1_tid) {
	char cmd[2];
	cmd[0] = 0;
	cmd[1] = train_id;
	Puts(com1_tid, cmd, 2);

	// Delay 2s then turn off the solenoid
	Delay(DELAY_REVERSE);

	// Reverse
	cmd[0] = TRAIN_REVERSE;
	Puts(com1_tid, cmd, 2);

	// Re-accelerate
	cmd[0] = new_speed;
	Puts(com1_tid, cmd, 2);
}

inline void setTrainSpeed(int train_id, int speed, int com1_tid) {
	char cmd[2];
	cmd[0] = speed;
	cmd[1] = train_id;
	Puts(com1_tid, cmd, 2);
}

inline int distToCM(int dist) {
	return (dist >> 14) / 10;
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

int stopCheck(track_node *cur_node, char *switch_table, int stop_distance, track_node *stop_node, track_node **route, int check_point, int com2_tid) {
	
	char str_buf[100];
	sprintf(str_buf, "stop node distance: %d\n", stop_distance);
	Puts(com2_tid, str_buf, 0);
	str_buf[0] = '\0';
	
	int distance = 0;
	int next_distance = 0;
	int direction;
	track_node *checked_node = cur_node;
	switch(checked_node->type) {
		case NODE_SENSOR:
		case NODE_MERGE:
			next_distance += (checked_node->edge[DIR_AHEAD].dist << 14);
			break;
		case NODE_BRANCH:
			direction = switch_table[switchIdToIndex(checked_node->num)] - 33;
			next_distance += (checked_node->edge[direction].dist << 14);
			break;
		default:
			return 0;
	}
	
	int hit_exit = 0;
	if (stop_node == NULL) {
		while (distance < (stop_distance + next_distance) && !hit_exit) {
			switch(checked_node->type) {
				case NODE_SENSOR:
				case NODE_MERGE:
					distance += (checked_node->edge[DIR_AHEAD].dist << 14);
					checked_node = checked_node->edge[DIR_AHEAD].dest;
					break;
				case NODE_BRANCH:
					direction = switch_table[switchIdToIndex(checked_node->num)] - 33;
					distance += (checked_node->edge[direction].dist << 14);
					checked_node = checked_node->edge[direction].dest;
					break;
				case NODE_EXIT:
					hit_exit = 1;
					break;
				default:
					bwprintf(COM2, "node type: %d\n", checked_node->type);
					assert(0, "stopCheck function cannot find a valid node type(1)");
					break;
			}
		}
	} else {
		for (; distance < (stop_distance + next_distance) && !hit_exit; check_point++) {
			if (stop_node == route[check_point]) {
				hit_exit = 1;
				break;
			}
			if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
				if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
					distance += ((route[check_point]->edge[DIR_STRAIGHT].dist) << 14);
				} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
					distance += ((route[check_point]->edge[DIR_CURVED].dist) << 14);
				} else {
					assert(0, "die ah sitra branch cannot find a dest");
				}
			} else {
				distance += (route[check_point]->edge[DIR_AHEAD].dist << 14);
			}
		}
	}
	
	if (hit_exit) {
		// int safe_dist = stop_distance - distance;
		// if (safe_dist > next_distance) {
			// assert(0, "WARNING!   TRAIN IS GONNA CRASH!");
			// return 0;
		// } else {
			// return next_distance - safe_dist;
		// }
		if (distance < stop_distance) {
			assert(0, "WARNING!   TRAIN IS GONNA CRASH!");
			return 0;
		} else {
			return distance - stop_distance;
		}
	}
	return -1;
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

int dijkstra(const track_node *track_nodes, const track_node *src,
              const track_node *dest, track_node **route) {
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
		if(u == dest) break;

		switch(u->type) {
			case NODE_ENTER:
			case NODE_SENSOR:
			case NODE_MERGE:
				alter_dist = u_dist + u->edge[DIR_AHEAD].dist;
				neighbour = u->edge[DIR_AHEAD].dest;
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
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

	assert(u == dest, "Dijkstra failed to find the destination");

	for(i = TRACK_MAX - 1; previous[u->index] != NULL; i--) {
		route[i] = u;
		u = previous[u->index];
	}
	route[i] = src;

	return i;
}

void changeNextSW(track_node **route, int check_point, char *switch_table, int com1_tid) {
	// char cmd[2];
	int distance = 0;
	// check_point += 1;
	int checked_switch = 0;
	for (; check_point < TRACK_MAX && distance < 1200; check_point++) {
		if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
			if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
					switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_STR;
					CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_STR, com1_tid, 0);
				}
				distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
			} else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
				if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
					switch_table[switchIdToIndex(route[check_point]->num)] = SWITCH_CUR;
					CreateWithArgs(2, switchChanger, route[check_point]->num, SWITCH_CUR, com1_tid, 0);
				}
				distance += route[check_point]->edge[DIR_CURVED].dist;
			} else {
				assert(0, "die ah sitra branch cannot find a dest");
			}
			checked_switch++;
		} else {
			distance += route[check_point]->edge[DIR_AHEAD].dist;
		}
	}
}
				
		

void trainDriver(TrainGlobal *train_global, TrainData *train_data) {
	track_node *track_nodes = train_global->track_nodes;
	track_node *predict_dest;
	track_node *stop_node = NULL;
	int tid, result;
	int train_id = train_data->id;
	int speed = 0;
	int old_speed = 0;
	int speed_before_reverse = 0;

	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;

	TrainMsg msg;
	unsigned int timer = 0; 		// 1/2000s
	unsigned int prev_timer = 0;	// 1/2000s
	unsigned int start_time = 0;
	char str_buf[1024];
	int forward_distance;	// zx unit
	
	unsigned int velocity_alarm = 0;
	// unsigned int position_alarm = 0;
	// unsigned int stop_alarm = 0;
	int dist_before_stop = -1; 		// zx unit, distance from current landmark and stop point
	unsigned int reverse_alarm = 0;
	// unsigned int freeze_point = 0;
	int acceleration = 0;
	int check_point = -1; 		// for finding route
	
	// tmp deaccelerate
	// train location initialize:
	train_data->landmark = &(track_nodes[0]);
	int direction;
	forward_distance = (getNextNodeDist(train_data->landmark, train_global->switch_table, &direction)) << 14;
	predict_dest = train_data->landmark->edge[direction].dest;
	
	int predict_location = 0;
	
	char *buf_cursor = str_buf;

	// Initialize trian speed
	setTrainSpeed(train_id, speed, com1_tid);

	// track_node *prev_landmark = &(track_nodes[0]);
	// track_node *cur_landmark = &(track_nodes[0]);
	int dist_traveled = -1;
	
	int secretary_tid = Create(2, trainSecretary);

	track_node *route[TRACK_MAX];
	int route_start = TRACK_MAX;

	int cnt = 0;
	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");
		if (tid == secretary_tid) {
			Reply(tid, NULL, 0);
			timer = getTimerValue(TIMER3_BASE);
			
			train_data->ahead_lm = train_data->ahead_lm + train_data->velocity * (prev_timer - timer);
			cnt++;
			train_data->velocity = train_data->velocity + (prev_timer - timer) * acceleration;
			if (acceleration > 0) {
				if (train_data->velocity > train_data->velocities[speed % 16]) {
					train_data->velocity = train_data->velocities[speed % 16];
					acceleration = 0;
				}
			} else if (acceleration < 0) {
				if (train_data->velocity < train_data->velocities[speed % 16]) {
					train_data->velocity = train_data->velocities[speed % 16];
					acceleration = 0;
				}
			}
			
			if (dist_before_stop != -1) {
				if (train_data->ahead_lm > dist_before_stop) {
					setTrainSpeed(train_id, 0, com1_tid);
					sprintf(str_buf, "stop node %s, distance: %d\n", train_data->landmark->name, dist_before_stop >> 14);
					Puts(com2_tid, str_buf, 0);
					str_buf[0] = '\0';
					dist_before_stop = -1;
					speed = 0;
					acceleration = -1;
				}
			}
				
			if (train_data->ahead_lm > forward_distance && predict_dest->type == NODE_EXIT) {
				train_data->landmark = predict_dest;
				train_data->ahead_lm = 0;
				forward_distance = 0;
			}
			
			if (train_data->ahead_lm > forward_distance && predict_location) {
				sprintf(str_buf, "prid reach: %s, %d, v: %d\n", predict_dest->name, cnt, train_data->velocity);
				Puts(com2_tid, str_buf, 0);
				str_buf[0] = '\0';
				cnt = 0;
				train_data->landmark = predict_dest;
				train_data->ahead_lm = train_data->ahead_lm - forward_distance;
				predict_location = 0;
				forward_distance = getNextNodeDist(train_data->landmark, train_global->switch_table, &direction);
				if (forward_distance > 0) {
					forward_distance <<= 14;
					predict_dest = train_data->landmark->edge[direction].dest;
					if (predict_dest->type != NODE_SENSOR) {
						predict_location = 1;
					}
				}
				if (dist_before_stop == -1 && speed%16 != 0) {
					int ret = stopCheck(train_data->landmark, train_global->switch_table, train_data->stop_dist[speed % 16] + (train_data->ht_length[train_data->direction] << 14), stop_node, route, check_point, com2_tid);
					if (ret >= 0) {
						dist_before_stop = ret;
					}
				}
				if (check_point != -1) {
					check_point += 1;
					if (check_point < TRACK_MAX) {
						if (train_data->landmark->type == NODE_SENSOR) {
							changeNextSW(route, check_point, train_global->switch_table, com1_tid);
						}
					} else {
						check_point = -1;
						stop_node = NULL;
					}
				}
			}
			
			if (reverse_alarm > timer) {
				if (train_data->direction == FORWARD) {
					train_data->direction = BACKWARD;
				} else {
					train_data->direction = FORWARD;
				}
				
				track_node *tmp;
				tmp = train_data->landmark;
				train_data->landmark = predict_dest->reverse;
				predict_dest = tmp->reverse;
				train_data->ahead_lm = forward_distance - train_data->ahead_lm;
				
				speed = speed_before_reverse;
				acceleration = 1;
				reverse_alarm = 0;
			}
			
			prev_timer = timer;
			// if (forward_distance && train_data->velocity) {
				// sprintf(str_buf, "%d, %d\n", train_data->ahead_lm >> 14, forward_distance - (train_data->ahead_lm >> 14));
				// Puts(com2_tid, str_buf, 0);
			// }
			continue;
		}
		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				
				old_speed = speed;
				speed = msg.cmd_msg.value;
				
				if (speed % 16 > old_speed % 16) {
					acceleration = 1;
				} else if (speed % 16 < old_speed % 16) {
					acceleration = -1;
				}
				
				setTrainSpeed(train_id, speed, com1_tid);
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				CreateWithArgs(2, trainReverser, train_id, speed, com1_tid, 0);
				acceleration = -1;
				speed_before_reverse = speed;
				speed = 0;
				
				start_time = getTimerValue(TIMER3_BASE);
				reverse_alarm = start_time - 4000;
				
				break;
			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				train_data->landmark = &(track_nodes[msg.location_msg.id]);
				sprintf(str_buf, "reach: %s, %d, v: %d\n", train_data->landmark->name, cnt, train_data->velocity);
				Puts(com2_tid, str_buf, 0);
				cnt = 0;
				// str_buf[0] = '\0';
				
				if (check_point > -1) {
					check_point += 1;
					if (check_point < TRACK_MAX) {
						if (route[check_point] != train_data->landmark) {
							sprintf(str_buf, "route node: %s, got node: %s\n", route[check_point]->name, train_data->landmark->name);
							Puts(com2_tid, str_buf, 0);
							// str_buf[0] = '\0';
							sprintf(str_buf, "recalculate route: %s -> %s\n", train_data->landmark->name, stop_node->name);
							Puts(com2_tid, str_buf, 0);
							route_start = dijkstra(track_nodes, train_data->landmark, stop_node, route);
							check_point = route_start;
							changeNextSW(route, check_point, train_global->switch_table, com1_tid);
							// buf_cursor += sprintf(buf_cursor, "%s -> ", train_data->landmark->name);
							for(; route_start < TRACK_MAX; route_start++) {
								buf_cursor += sprintf(buf_cursor, "%s -> ", route[route_start]->name);
							}
							buf_cursor += sprintf(buf_cursor, "\n");
							Puts(com2_tid, str_buf, 0);
							str_buf[0] = '\0';
						} else {
							if (train_data->landmark->type == NODE_SENSOR) {
								changeNextSW(route, check_point, train_global->switch_table, com1_tid);
							}
						}
						
					} else {
						check_point = -1;
						stop_node = NULL;
					}
				}
				train_data->ahead_lm = 0;
				
				predict_location = 0;
				forward_distance = getNextNodeDist(train_data->landmark, train_global->switch_table, &direction);
				if (forward_distance > 0) {
					forward_distance <<= 14;
					predict_dest = train_data->landmark->edge[direction].dest;
					if (predict_dest->type != NODE_SENSOR) {
						predict_location = 1;
					}
				}
				if (dist_before_stop == -1 && speed%16 != 0) {
					int ret = stopCheck(train_data->landmark, train_global->switch_table, train_data->stop_dist[speed % 16] + (train_data->ht_length[train_data->direction] << 14), stop_node, route, check_point, com2_tid);
					if (ret >= 0) {
						dist_before_stop = ret;
					}
				}
				break;
			case CMD_GOTO:
				Reply(tid, NULL, 0);
				sprintf(str_buf, "T#%d routing to %s\n", train_id, track_nodes[msg.location_msg.value].name);
				Puts(com2_tid, str_buf, 0);

				stop_node = &(track_nodes[msg.location_msg.value]);

				route_start = dijkstra(track_nodes, train_data->landmark, &(track_nodes[msg.location_msg.value]), route);
				check_point = route_start;
				// buf_cursor += sprintf(buf_cursor, "%s -> ", train_data->landmark->name);
				for(; route_start < TRACK_MAX; route_start++) {
					buf_cursor += sprintf(buf_cursor, "%s -> ", route[route_start]->name);
				}
				buf_cursor += sprintf(buf_cursor, "\n");
				Puts(com2_tid, str_buf, 0);
				str_buf[0] = '\0';
				
				changeNextSW(route, check_point, train_global->switch_table, com1_tid);
				// if (check_point != -1) {
					// sprintf(str_buf, "branch change: %s\n", route[check_point]->name);
					// Puts(com2_tid, str_buf, 0);
				// }
				
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

