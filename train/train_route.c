#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>
#include <ts7200.h>

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
              track_node *dest, track_node **route, int *dist, int *map_record) {
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
				neighbour = u->edge[DIR_AHEAD].dest;
				alter_dist = u_dist + u->edge[DIR_AHEAD].dist;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = alter_dist * 4;
				}
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			case NODE_MERGE:
				neighbour = u->edge[DIR_AHEAD].dest;
				alter_dist = u_dist + u->edge[DIR_AHEAD].dist;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = alter_dist * 4;
				}
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				neighbour = u->reverse->edge[DIR_STRAIGHT].dest;
				alter_dist = u_dist + u->reverse->edge[DIR_STRAIGHT].dist + 400;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = (alter_dist - 400) * 4 + 400;
				}
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				neighbour = u->reverse->edge[DIR_CURVED].dest;
				alter_dist = u_dist + u->reverse->edge[DIR_CURVED].dist + 400;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = (alter_dist - 400) * 4 + 400;
				}
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);
				break;
			case NODE_BRANCH:
				neighbour = u->edge[DIR_STRAIGHT].dest;
				alter_dist = u_dist + u->edge[DIR_STRAIGHT].dist;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = alter_dist * 4;
				}
				dijkstra_update(&dist_heap, heap_nodes, previous, u, neighbour, alter_dist);

				neighbour = u->edge[DIR_CURVED].dest;
				alter_dist = u_dist + u->edge[DIR_CURVED].dist;
				if ((map_record[u->index] && map_record[neighbour->index])
						|| (map_record[u->reverse->index] && map_record[neighbour->reverse->index])) {
						alter_dist = alter_dist * 4;
				}
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

// return route start
int findRoute(track_node *track_nodes, TrainData *train_data, track_node *dest, int *map_record, int *need_reverse) {
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
		forward_start = dijkstra(track_nodes, train_data->landmark, dest, forward_route, &forward_dist, map_record);
		backward_start = dijkstra(track_nodes, train_data->landmark->reverse, dest, backward_route, &backward_dist, map_record);
	} else {
		forward_start = dijkstra(track_nodes, train_data->predict_dest, dest, forward_route, &forward_dist, map_record);
		backward_start = dijkstra(track_nodes, train_data->predict_dest->reverse, dest, backward_route, &backward_dist, map_record);
	}

	// iprintf(4, 40, "\e[s\e[20;2Hf: %d, fs: %d, b: %d, bs: %d\e[u", forward_dist - (find_stop_dist(train_data) >> DIST_SHIFT), forward_start, backward_dist + (find_stop_dist(train_data) >> DIST_SHIFT), backward_start);
	if (forward_dist - 2 * (find_stop_dist(train_data) >> DIST_SHIFT) - 2 * (train_data->ahead_lm >> DIST_SHIFT) < backward_dist) {
		*need_reverse = 0;
		int i;
		for (i = forward_start; i < TRACK_MAX; i++) {
			train_data->route[i] = forward_route[i];
		}
		return forward_start;
	} else {
		*need_reverse = 1;
		int i;
		for (i = backward_start; i < TRACK_MAX; i++) {
			train_data->route[i] = backward_route[i];
		}
		return backward_start;
	}
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

void routeDelivery(int train_tid, TrainMsgType msgType) {
	int result;
	TrainMsgType msg;
	msg = msgType;
	result = Send(train_tid, (char *)(&msg), sizeof(TrainMsgType), NULL, 0);
	assert(result == 0, "routeDelivery fail to deliver route");
}

void clearRoute(int *map_record, track_node **route, int route_start) {
	int i;
	for (i = route_start; i < TRACK_MAX; i++) {
		map_record[route[i]->index] -= 1;
		// assert(map_record[i] >= 0, "invalid map clearance");
	}
}

void markRoute(int *map_record, track_node **route, int route_start) {
	int i;
	for (i = route_start; i < TRACK_MAX; i++) {
		map_record[route[i]->index] += 1;
		// assert(map_record[i] >= 0, "invalid map clearance");
	}
}

void routeServer(TrainGlobal *train_global) {
	int i, tid, result, need_reverse, com1_tid, com2_tid, tmp_dist;
	com1_tid = train_global->com1_tid;
	com2_tid = train_global->com2_tid;
	
	int map_record[TRACK_MAX];
	
	for (i = 0; i < TRACK_MAX; i++) {
		map_record[i] = 0;
	}
	RouteMsg msg;
	while(TRUE) {
		result = Receive(&tid, (char *)(&msg), sizeof(RouteMsg));
		assert(result >= 0, "routeServer receive failed");
		Reply(tid, NULL, 0);
		switch (msg.type) {
			case FIND_ROUTE:
				clearRoute(map_record, msg.train_data->route, msg.train_data->route_start);
				msg.train_data->route_start = findRoute(train_global->track_nodes, msg.train_data, msg.destination, map_record, &need_reverse);
				msg.train_data->stop_node = msg.train_data->route[TRACK_MAX - 1];
				if (need_reverse) {
					markRoute(map_record, msg.train_data->route, msg.train_data->route_start);
					CreateWithArgs(2, routeDelivery, msg.train_data->tid, NEED_REVERSE, 0, 0);
				} else {
					if (find_reverse_node(msg.train_data, train_global, msg.train_data->route_start)) {
						msg.train_data->route_start = dijkstra(train_global->track_nodes, msg.train_data->landmark, msg.train_data->reverse_node, msg.train_data->route, &tmp_dist, map_record);
						markRoute(map_record, msg.train_data->route, msg.train_data->route_start);
						CreateWithArgs(2, routeDelivery, msg.train_data->tid, GOTO_MERGE, 0, 0);
					} else {
						markRoute(map_record, msg.train_data->route, msg.train_data->route_start);
						CreateWithArgs(2, routeDelivery, msg.train_data->tid, GOTO_DEST, 0, 0);
					}
				}
				break;
				
			case CLEAR_ROUTE:
				clearRoute(map_record, msg.train_data->route, msg.train_data->route_start);
				msg.train_data->route_start = TRACK_MAX;
				break;
				
			default:
				assert(0, "routeServer receive invalid msg type");
		}
	}
}
