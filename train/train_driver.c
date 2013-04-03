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

void trainDriver(TrainGlobal *train_global, TrainProperties *train_properties) {
	track_node *track_nodes = train_global->track_nodes;
	int tid, result;
	int train_id = train_properties->id;
	int speed = 0;

	int com1_tid = train_global->com1_tid;
	int com2_tid = train_global->com2_tid;

	TrainMsg msg;
	char str_buf[1024];

	// Initialize trian speed
	setTrainSpeed(train_id, speed, com1_tid);

	track_node *prev_landmark = &(track_nodes[0]);
	track_node *cur_landmark = &(track_nodes[0]);
	unsigned int t1 = 0xffffffff;
	unsigned int t2 = 0;
	unsigned int dist_traveled = -1;
	int ss_cnt = 0;
	int v_list[15];
	int i;

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");

		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				speed = msg.cmd_msg.value;
				setTrainSpeed(train_id, speed, com1_tid);
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				CreateWithArgs(2, trainReverser, train_id, speed, com1_tid, 0);
				break;
			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				if(msg.location_msg.value) {
					ss_cnt++;
					if (ss_cnt == 10) {
						t1 = getTimerValue(TIMER3_BASE);
						dist_traveled = 0;
					} else if (ss_cnt == 200 + speed * 5) {
						t2 = getTimerValue(TIMER3_BASE);
						v_list[speed] = (((dist_traveled << 15) / (t1 - t2)) << 3);
						sprintf(str_buf, "speed %d: %u \n", speed, v_list[speed]);
						Puts(com2_tid, str_buf, 0);
						str_buf[0] = '\0';
						ss_cnt = 0;
						speed++;
						if (speed == 15) {
							setTrainSpeed(train_id, 0, com1_tid);
						} else {
							setTrainSpeed(train_id, speed, com1_tid);
						}
					}
					cur_landmark = &(track_nodes[msg.location_msg.id]);
					dist_traveled += calcDistance(prev_landmark, cur_landmark, 5);
					prev_landmark = cur_landmark;
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

