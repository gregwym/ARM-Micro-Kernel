#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

#define DELAY_REVERSE 200
#define RESERVE_CHECKPOINT_LEN 50
#define RESERVER_PRIORITY 7

int measureDist(TrainData *train_data) {
	int dist = 0;
	int direction;
	track_node *checked_node = train_data->orbit->orbit_start;
	while(checked_node != train_data->landmark) {
		switch(checked_node->type) {
			case NODE_ENTER:
			case NODE_SENSOR:
			case NODE_MERGE:
				dist += checked_node->edge[DIR_AHEAD].dist;
				checked_node = checked_node->edge[DIR_AHEAD].dest;
				break;
			case NODE_BRANCH:
				direction = train_data->orbit->orbit_switches[switchIdToIndex(checked_node->num)] - 33;
				dist += checked_node->edge[direction].dist;
				checked_node = checked_node->edge[direction].dest;
				break;
			case NODE_EXIT:
				assert(0, "orbit1 hit an exit point");
			default:
				break;
		}
	}
	dist += (train_data->ahead_lm >> DIST_SHIFT);
	return dist;
}

int isOnOrbit(TrainData *train_data) {
	int i;
	track_node **route = train_data->orbit->orbit_route;
	for (i = 0; i < train_data->orbit->nodes_num; i++) {
		if(train_data->landmark == route[i]) return 1;
	}
	return 0;
}

inline void dijkstraUpdate(Heap *dist_heap, HeapNode *heap_nodes, track_node **previous,
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

int dijkstra(TrainData *train_data, track_node *track_nodes, track_node *src,
              track_node *dest, track_node **route) {
	if (src == NULL || dest == NULL) {
		assert(0, "dijkstra get null src or dest");
		return TRACK_MAX;
	}
	int i, j;
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
				dijkstraUpdate(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			case NODE_BRANCH:
				alter_dist = u_dist + u->edge[DIR_STRAIGHT].dist;
				neighbour = u->edge[DIR_STRAIGHT].dest;
				dijkstraUpdate(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				alter_dist = u_dist + u->edge[DIR_CURVED].dist;
				neighbour = u->edge[DIR_CURVED].dest;
				dijkstraUpdate(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			default:
				break;
		}
	}

	// assert(u == dest, "Dijkstra failed to find the destination");
	if (u != dest) return TRACK_MAX;

	for(i = TRACK_MAX - 1; previous[u->index] != NULL; i--) {
		route[i] = u;
		u = previous[u->index];
	}
	route[i] = src;
	if (src != train_data->landmark) {
		route[i - 1] = train_data->landmark;
		i--;
	}
	
	for(j = 0; i < TRACK_MAX; i++, j++) {
		route[j] = route[i];
	}

	return j;
}

track_node *predictNextSensor(TrainGlobal *train_global, TrainData *train_data) {
	int direction;
	track_node *checked_node = train_data->landmark;
	checked_node = checked_node->edge[DIR_AHEAD].dest;
	int loop = 1;
	if (train_data->action == To_Orbit) {
		track_node **route = train_data->route;
		int check_point = train_data->check_point;
		assert(train_data->landmark == route[check_point], "check point does not match cur landmark");
		check_point++;
		while (check_point < train_data->route_nodes_num - 1) {
			switch(route[check_point]->type) {
				case NODE_ENTER:
				case NODE_MERGE:
				case NODE_BRANCH:
					check_point++;
					break;
				case NODE_SENSOR:
					return route[check_point];
				case NODE_EXIT:
					assert(0, "predictNextSensor hit exit(1)");
				default:
					break;
			}			
		}
	} else {
		while (loop) {
			switch(checked_node->type) {
				case NODE_ENTER:
				case NODE_MERGE:
					checked_node = checked_node->edge[DIR_AHEAD].dest;
					break;
				case NODE_BRANCH:
					direction = train_data->orbit->orbit_switches[switchIdToIndex(checked_node->num)] - 33;
					checked_node = checked_node->edge[direction].dest;
					break;
				case NODE_SENSOR:
					return checked_node;
					break;
				case NODE_EXIT:
					assert(0, "predictNextSensor hit exit(2)");
				default:
					break;
			}
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

// void routeRequest(TrainGlobal *train_global, TrainData *train_data, track_node *dest) {
	// int result;
	// RouteMsg msg;
	// msg.type = FIND_ROUTE;
	// msg.train_data = train_data;
	// msg.destination = dest;
	// result = Send(train_global->route_server_tid, (char *)(&msg), sizeof(RouteMsg), NULL, 0);
	// assert(result == 0, "routeDelivery fail to deliver route");
// }

void stReport(TrainGlobal *train_global, TrainData *train_data) {
	int result;
	SatelliteReport msg;
	msg.type = SATELLITE_REPORT;
	msg.train_data = train_data;
	msg.orbit = train_data->orbit;
	msg.distance = train_data->dist_traveled;
	msg.next_sensor = train_data->next_sensor;
	msg.parent_train = train_data->parent_train;
	result = Send(train_global->center_tid, (char *)(&msg), sizeof(SatelliteReport), (char *)(&msg), sizeof(SatelliteReport));
	assert(result >= 0, "stReport fail to receive center reply");
	if (result != 0) {
		result = Send(MyParentTid(), (char *)(&msg), sizeof(SatelliteReport), NULL, 0);
		assert(result == 0, "stReport fail to send to driver");
	} else {
		msg.type = SATELLITE_REPORT;
		result = Send(MyParentTid(), (char *)(&(msg.type)), sizeof(TrainMsgType), NULL, 0);
		assert(result == 0, "stReport fail to send to driver");
	}
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

void trainSecretary() {
	int parent_tid = MyParentTid();
	char ch = '1';
	while(1) {
		Delay(2);
		Send(parent_tid, &ch, 0, NULL, 0);
	}
}

void changeNextSwitch(TrainGlobal *train_global, TrainData *train_data, int check_point, int threshold) {
	int ahead_lm = train_data->ahead_lm >> DIST_SHIFT;
	int distance = 0;
	if (train_data->action == On_Orbit) {
		int nodes_num = train_data->orbit->nodes_num;
		check_point = (check_point + 1) % nodes_num;
		track_node **route = train_data->orbit->orbit_route;
		for (;distance < threshold + ahead_lm; check_point = (check_point + 1) % nodes_num) {
			if (route[check_point]->type == NODE_BRANCH) {
				if (route[(check_point + 1) % nodes_num] == route[check_point]->edge[DIR_STRAIGHT].dest) {
					if (train_global->switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
					}
					distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
				} else {
					if (train_global->switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
						IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					}
					distance += route[check_point]->edge[DIR_CURVED].dist;
				}
			} else {
				distance += route[check_point]->edge[DIR_AHEAD].dist;
			}
		}
	} else if (train_data->action == To_Orbit) {
		int nodes_num = train_data->route_nodes_num;
		check_point = check_point + 1;
		track_node **route = train_data->route;
		for (;distance < threshold + ahead_lm && check_point < nodes_num - 1; check_point++) {
			if (route[check_point]->type == NODE_BRANCH) {
				if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
					if (train_global->switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
					}
					distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
				} else {
					if (train_global->switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
						CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
						IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					}
					distance += route[check_point]->edge[DIR_CURVED].dist;
				}
			} else {
				distance += route[check_point]->edge[DIR_AHEAD].dist;
			}
		}
	}	
}
// void changeNextSW(TrainGlobal *train_global, TrainData *train_data, int check_point, char *switch_table, int threshold) {
	// char cmd[2];
	// int distance = 0;
	// check_point += 1;
	// track_node **route = train_data->route;
	// int ahead_lm = train_data->ahead_lm >> DIST_SHIFT;
	// for (; check_point >= 0 && check_point < TRACK_MAX && distance < threshold + ahead_lm; check_point++) {
		// if (route[check_point]->type == NODE_BRANCH && check_point + 1 < TRACK_MAX) {
			// if (route[check_point + 1] == route[check_point]->edge[DIR_STRAIGHT].dest) {
				// if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
					// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
					// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
				// }
				// distance += (route[check_point]->edge)[DIR_STRAIGHT].dist;
			// } else if (route[check_point + 1] == route[check_point]->edge[DIR_CURVED].dest) {
				// if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
					// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
					// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
				// }
				// distance += route[check_point]->edge[DIR_CURVED].dist;
			// } else {
				// assert(0, "fail to change switch");
			// }
		// } else if (route[check_point]->type == NODE_MERGE && check_point + 1 < TRACK_MAX) {
			// if (route[check_point + 1] != route[check_point]->edge[DIR_AHEAD].dest) {
				// if (route[check_point]->reverse->edge[DIR_STRAIGHT].dest == route[check_point + 1]) {
					// if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_STR) {
						// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_STR, 0);
						// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					// }
					// distance += (route[check_point]->reverse->edge)[DIR_STRAIGHT].dist;
				// } else if (route[check_point]->reverse->edge[DIR_CURVED].dest == route[check_point + 1]) {
					// if (switch_table[switchIdToIndex(route[check_point]->num)] != SWITCH_CUR) {
						// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->num, SWITCH_CUR, 0);
						// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
					// }
					// distance += (route[check_point]->reverse->edge)[DIR_CURVED].dist;
				// } else {
					// assert(0, "fail to change switch (1)");
				// }
				// if (route[check_point]->edge[DIR_AHEAD].dest->type == NODE_MERGE && route[check_point]->edge[DIR_AHEAD].dist < 240) {
					// if (route[check_point]->edge[DIR_AHEAD].dest->reverse->edge[DIR_STRAIGHT].dest == route[check_point]->reverse) {
						// if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_STR) {
							// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_STR, 0);
							// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
						// }
					// } else {
						// if (switch_table[switchIdToIndex(route[check_point]->edge[DIR_AHEAD].dest->num)] != SWITCH_CUR) {
							// CreateWithArgs(2, switchChanger, (int)train_global, route[check_point]->edge[DIR_AHEAD].dest->num, SWITCH_CUR, 0);

							// IDEBUG(DB_ROUTE, train_global->com2_tid, 49, 2 + train_data->index * 40, "%d change %d  ", train_data->id, route[check_point]->num);
						// }
					// }
				// }
			// } else {
				// distance += route[check_point]->edge[DIR_AHEAD].dist;
			// }
		// } else {
			// distance += route[check_point]->edge[DIR_AHEAD].dist;
		// }
	// }
// }

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

// return check point forward by 1, and adjust it if it does not match current landmark
// return -1 if current landmark 6is not in the route
int updateCheckPoint(TrainData *train_data, track_node **route, int check_point) {
	assert(check_point != -1, "check point is -1 in updateCheckPoint");
	int i;
	int nodes_num = train_data->orbit->nodes_num;
	if (check_point < 0) {
		return -1;
	} else if (check_point + 1 < nodes_num) {
		check_point += 1;
		if (route[check_point] != train_data->landmark) {
			for (i = 0; i < nodes_num; i++) {
				if (route[i] == train_data->landmark) {
					return i;
				}
			}
			return -1;
		} else {
			return check_point;
		}
	} else {
		for (i = 0; i < nodes_num; i++) {
			if (route[i] == train_data->landmark) {
				return i;
			}
		}
		return -1;
	}
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
	int ret;
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
		if (train_data->predict_dest->type != NODE_SENSOR) {
			updateCurrentLandmark(train_global, train_data, NULL, train_global->switch_table);
			if (train_data->action == On_Orbit) {
				ret = updateCheckPoint(train_data, train_data->orbit->orbit_route, train_data->check_point);
				assert(ret >= 0, "current landmark is not on orbit");
				train_data->check_point = ret;
			} else if (train_data->action == To_Orbit) {
				ret = updateCheckPoint(train_data, train_data->route, train_data->check_point);
				if (ret == -1) {
					uiprintf(train_global->com2_tid, 55, train_data->index * 40, "off route when updateTrainStatus");
					train_data->action = Off_Route;
				} else {
					train_data->check_point = ret;
				}
			}	
		} else {
			train_data->sensor_timeout = train_data->timer - 1200;
		}
	}
	
	if (train_data->timer < train_data->sensor_timeout) {
		CreateWithArgs(2, relocationRequest, (int)train_global, (int)train_data, 0, 0);
	}
		
}

// void updateReservation(TrainGlobal *train_global, TrainData *train_data) {
	// if (train_data->last_reservation_time - train_data->timer > 400) {
		// changeNextSW(train_global, train_data, train_data->check_point, train_global->switch_table, find_stop_dist(train_data) >> DIST_SHIFT);
		// if (train_data->waiting_for_reserver) {
			// changeSpeed(train_global, train_data, 0);
			// train_data->acceleration = train_data->deceleration;
			// train_data->stop_type = RB_slowing;
			// train_data->dist_since_last_rs = 0;
		// } else {
			// if (train_data->velocity != 0) {
				// if (train_data->action != RB_last_ss && train_data->action != Off_Route) {
					// reserveInRunning(train_global, train_data);
				// }
			// } else {
				// if (train_data->action == RB_last_ss) {
					// reserveInRecovery(train_global, train_data);
				// } else {
					// reserveInRunning(train_global, train_data);
					// reserveInRunning(train_global, train_data);
				// }
			// }
		// }
	// }
// }



void updateReport(TrainGlobal *train_global, TrainData *train_data) {
	if (train_data->last_report_time - train_data->timer > 400 && !train_data->waiting_for_reporter) {
		if (train_data->action == On_Orbit) {
			train_data->dist_traveled = measureDist(train_data);
		}
		CreateWithArgs(2, stReport, (int)train_global, (int)train_data, 0, 0);
		train_data->waiting_for_reporter = TRUE;
		train_data->last_report_time = train_data->timer;
	}
}


void trainDriver(TrainGlobal *train_global, TrainData *train_data) {
	track_node *track_nodes = train_global->track_nodes;
	int tid, result, route_nodes, cnt = 0;
	int train_id = train_data->id;
	int train_index = train_data->index;

	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;
	TrainMsg msg;

	// initialize start position
	updateCurrentLandmark(train_global, train_data,
	                      &(track_nodes[train_data->reservation_record.landmark_id]),
	                      train_global->switch_table);

	// Initialize train speed
	setTrainSpeed(train_id, train_data->speed, com1_tid);

	int secretary_tid = Create(2, trainSecretary);	
	train_data->next_sensor = predictNextSensor(train_global, train_data);

	Delay(710);
	
	int expect_dist_diff = 0;
	int actual_dist_diff = 0;
	Orbit *new_orbit = NULL;
	int parent_percentage;
	int self_percentage;

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");
		if (tid == secretary_tid) {
			Reply(tid, NULL, 0);
			train_data->timer = getTimerValue(TIMER3_BASE);

			updateTrainStatus(train_global, train_data);
			updateReport(train_global, train_data);
			changeNextSwitch(train_global, train_data, train_data->check_point, 70);

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
				break;

			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				changeSpeed(train_global, train_data, 0);
				break;
				
			case CMD_SET:
				updateCurrentLandmark(train_global, train_data, &(track_nodes[msg.location_msg.value]), train_global->switch_table);
				break;
			case CMD_ORBIT:
				new_orbit = &(train_global->orbits[msg.cmd_msg.value]);
				if (train_data->orbit != new_orbit) {
					train_data->orbit = new_orbit;
					if (train_data->landmark->type != NODE_BRANCH) {
						route_nodes = dijkstra(train_data, track_nodes, train_data->landmark, new_orbit->orbit_start, train_data->route);
					} else {
						route_nodes = dijkstra(train_data, track_nodes, train_data->predict_dest, new_orbit->orbit_start, train_data->route);
					}
					if (route_nodes == 0) {
						assert(0, "dijkstra cannot find route");
					} else {
						train_data->route_nodes_num = route_nodes;
						train_data->check_point = 0;
						train_data->action = To_Orbit;
					}
					changeSpeed(train_global, train_data, 26);
					// check_point = route_start;
				}
				break;
			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);

				train_data->last_receive_sensor = &(track_nodes[msg.location_msg.id]);

				updateCurrentLandmark(train_global, train_data, &(track_nodes[msg.location_msg.id]), train_global->switch_table);
				if (train_data->action != On_Orbit) {
					if (isOnOrbit(train_data)) {
						train_data->action = On_Orbit;
						result = updateCheckPoint(train_data, train_data->orbit->orbit_route, 0);
						assert(result >= 0, "current landmark is not on orbit");
						train_data->check_point = result;
					} else {
						if (train_data->action == To_Orbit) {
							result = updateCheckPoint(train_data, train_data->route, 0);
							if (result == -1) {
								uiprintf(com2_tid, 56, train_data->index * 40, "Off_route");
								train_data->action = Off_Route;
								// todo
							} else {
								train_data->check_point = result;
							}
						}
					}
				}
				if (train_data->action == On_Orbit) {
					train_data->dist_traveled = measureDist(train_data);
					result = updateCheckPoint(train_data, train_data->orbit->orbit_route, train_data->check_point);
					assert(result >= 0, "current landmark is not on orbit");
					train_data->check_point = result;
					changeNextSwitch(train_global, train_data, train_data->check_point, 70);
				}
				train_data->next_sensor = predictNextSensor(train_global, train_data);
				train_data->sensor_timeout = 0;
				break;
			
			case SATELLITE_REPORT:
				Reply(tid, NULL, 0);
				train_data->waiting_for_reporter = FALSE;
				if (train_data->parent_train != NULL && train_data->action == On_Orbit && train_data->parent_train->action == On_Orbit) {
					if (train_data->parent_train->orbit == train_data->orbit) {
						if (train_data->follow_mode == Percentage) {
							expect_dist_diff = (train_data->orbit->orbit_length * train_data->follow_percentage) / 100;
						} else {
							expect_dist_diff = train_data->follow_dist;
						}
						actual_dist_diff = (msg.satellite_report.distance + train_data->orbit->orbit_length - train_data->dist_traveled) % train_data->orbit->orbit_length;
						result = actual_dist_diff - expect_dist_diff;
						if (result > 200) {
							changeSpeed(train_global, train_data, 28);
						} else if (result > 0) {
							changeSpeed(train_global, train_data, 27);
						} else if (result < -200) {
							changeSpeed(train_global, train_data, 8);
						} else if (result < 0) {
							changeSpeed(train_global, train_data, 9);
						} else {
							changeSpeed(train_global, train_data, 26);
						}
					} else {
						if (train_data->parent_train->orbit->orbit_length - train_data->orbit->orbit_length > 1400) {
							parent_percentage = (((msg.satellite_report.distance * 2) % train_data->parent_train->orbit->orbit_length) * 100) / train_data->parent_train->orbit->orbit_length;
							self_percentage = (train_data->dist_traveled * 100) / train_data->orbit->orbit_length;
						} else if (train_data->parent_train->orbit->orbit_length - train_data->orbit->orbit_length < -1400) {
							parent_percentage = (msg.satellite_report.distance * 100) / train_data->parent_train->orbit->orbit_length;
							self_percentage = (((train_data->dist_traveled * 2) % train_data->orbit->orbit_length) * 100) / train_data->orbit->orbit_length;
						} else {
							parent_percentage = (msg.satellite_report.distance * 100) / train_data->parent_train->orbit->orbit_length;
							self_percentage = (train_data->dist_traveled * 100) / train_data->orbit->orbit_length;
						}
						result = (parent_percentage + 100 - self_percentage) % 100;
						if (result > train_data->follow_percentage + 15) {
							changeSpeed(train_global, train_data, 28);
						} else if (result > train_data->follow_percentage) {
							changeSpeed(train_global, train_data, 27);
						} else if (result < train_data->follow_percentage - 15) {
							changeSpeed(train_global, train_data, 8);
						} else if (result < train_data->follow_percentage) {
							changeSpeed(train_global, train_data, 9);
						} else {
							changeSpeed(train_global, train_data, 26);
						}
					}
				}
				if (train_data->action == On_Orbit) {
					uiprintf(com2_tid, 36, train_data->index * 40 + 2, "%d  ", train_data->dist_traveled);
					uiprintf(com2_tid, 37, train_data->index * 40 + 2, "%d  ", (train_data->dist_traveled * 100) / train_data->orbit->orbit_length);
				}
				break;
			
			default:
				assert(0, "Driver got unknown msg, type\n");
				break;
		}
	}
}
