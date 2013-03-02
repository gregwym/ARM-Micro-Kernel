#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

#define DELAY_REVERSE 200

void trainReverser(int train_id, int new_speed) {
	char cmd[2];
	cmd[0] = 0;
	cmd[1] = train_id;
	Puts(COM1, cmd, 2);

	// Delay 2s then turn off the solenoid
	Delay(DELAY_REVERSE);

	// Reverse
	cmd[0] = TRAIN_REVERSE;
	Puts(COM1, cmd, 2);

	// Re-accelerate
	cmd[0] = new_speed;
	Puts(COM1, cmd, 2);
}

inline void setTrainSpeed(int train_id, int speed) {
	char cmd[2];
	cmd[0] = speed;
	cmd[1] = train_id;
	Puts(COM1, cmd, 2);
}

void trainDriver(TrainGlobal *train_global, TrainProperties *train_properties) {
	track_node *track_nodes = train_global->track_nodes;
	int tid, result;
	int train_id = train_properties->id;
	int speed = 0;

	TrainMsg msg;
	char str_buf[1024];

	// Initialize trian speed
	setTrainSpeed(train_id, speed);

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainDriver receive failed");

		switch (msg.type) {
			case CMD_SPEED:
				Reply(tid, NULL, 0);
				speed = msg.cmd_msg.value;
				setTrainSpeed(train_id, speed);
				break;
			case CMD_REVERSE:
				Reply(tid, NULL, 0);
				CreateWithArgs(2, trainReverser, train_id, speed, 0, 0);
				break;
			case LOCATION_CHANGE:
				Reply(tid, NULL, 0);
				if(msg.location_msg.value) {
					sprintf(str_buf, "T#%d -> %s\n", train_id, track_nodes[msg.location_msg.id].name);
					Puts(COM2, str_buf, 0);
					if(msg.location_msg.id == 71) {
						speed = 0;
						setTrainSpeed(train_id, speed);
					}
				}
				break;
			default:
				sprintf(str_buf, "Driver got unknown msg, type: %d\n", msg.type);
				Puts(COM2, str_buf, 0);
				break;
		}

		str_buf[0] = '\0';
	}
}

