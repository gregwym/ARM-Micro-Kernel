#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

#define DELAY_REVERSE 200

//UI
#define LOCATION

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

void trainSecretary() {
	int parent_tid = MyParentTid();
	char ch = '1'; 
	while(1) {
		Delay(10);
		Send(parent_tid, &ch, 0, NULL, 0);
	}
}
		

void trainDriver(TrainGlobal *train_global, TrainData *train_data) {
	track_node *track_nodes = train_global->track_nodes;
	track_node *predict_dest;
	int tid, result;
	int train_id = train_data->id;
	int speed = 0;

	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;

	TrainMsg msg;
	unsigned int timer = 0; 	// 0.1s
	unsigned int start_time = 0;
	char str_buf[1024];
	int forward_distance;
	
	unsigned int velocity_alarm = 0;
	unsigned int position_alarm = 0;
	
	// Initialize trian speed
	setTrainSpeed(train_id, speed, com1_tid);

	// track_node *prev_landmark = &(track_nodes[0]);
	// track_node *cur_landmark = &(track_nodes[0]);
	int dist_traveled = -1;
	
	int secretary_tid = Create(2, trainSecretary);

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");
		if (tid == secretary_tid) {
			Reply(tid, NULL, 0);
			timer = getTimerValue(TIMER3_BASE);
			train_data->ahead_lm = (start_time - timer) * train_data->velocity;
			if (velocity_alarm > timer) {
				// todo
			}
			if (position_alarm > timer) {
				train_data->landmark = predict_dest;
				sprintf(str_buf, "reach: %s\n", train_data->landmark->name);
				Puts(com2_tid, str_buf, 0);
				str_buf[0] = '\0';
				train_data->ahead_lm = 0;
				if (train_data->velocity != 0) {
					int direction;
					start_time = getTimerValue(TIMER3_BASE);
					forward_distance = getNextNodeDist(train_data->landmark, train_global->switch_table, &direction);
					position_alarm = start_time - (forward_distance << 14) / train_data->velocity ;
					predict_dest = train_data->landmark->edge[direction].dest;
				}
			}
			if (forward_distance) {
				sprintf(str_buf, "%d, %d\n", train_data->ahead_lm >> 14, forward_distance - (train_data->ahead_lm >> 14));
				Puts(com2_tid, str_buf, 0);
			}
			continue;
		}
		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				speed = msg.cmd_msg.value;
				train_data->velocity = train_data->velocities[speed];
				setTrainSpeed(train_id, speed, com1_tid);
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				CreateWithArgs(2, trainReverser, train_id, speed, com1_tid, 0);
				break;
			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				if(msg.location_msg.value) {
					// cur_landmark = &(track_nodes[msg.location_msg.id]);
					// dist_traveled = calcDistance(prev_landmark, cur_landmark, 4, 0);
					// prev_landmark = cur_landmark;
					// sprintf(str_buf, "T#%d -> %s, traveled: %d\n", train_id, track_nodes[msg.location_msg.id].name, dist_traveled);
					
					train_data->landmark = &(track_nodes[msg.location_msg.id]);
					sprintf(str_buf, "reach: %s\n", train_data->landmark->name);
					Puts(com2_tid, str_buf, 0);
					train_data->ahead_lm = 0;
					if (train_data->velocity != 0) {
						int direction;
						start_time = getTimerValue(TIMER3_BASE);
						forward_distance = getNextNodeDist(train_data->landmark, train_global->switch_table, &direction);
						position_alarm = start_time - (forward_distance << 14) / train_data->velocity ;
						predict_dest = train_data->landmark->edge[direction].dest;
					}
				}
				break;
			default:
				sprintf(str_buf, "Driver got unknown msg, type: %d\n", msg.type);
				Puts(com2_tid, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}

