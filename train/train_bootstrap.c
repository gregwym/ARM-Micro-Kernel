#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

void trainBootstrap() {

	int tid;

	/* Track Data */
	track_node track_nodes[TRACK_MAX];
	init_trackb(track_nodes);

	/* Switch Data */
	char switch_table[SWITCH_TOTAL];
	memset(switch_table, SWITCH_CUR, SWITCH_TOTAL);

	/* Train Data */
	TrainData train_data[TRAIN_MAX];
	init_train37(&(train_data[0]));
	init_train49(&(train_data[1]));

	/* Track Reservation */
	int track_reservation[TRACK_MAX];
	memset(track_reservation, -1, sizeof(int) * TRACK_MAX);

	/* Train Global */
	TrainGlobal train_global;
	train_global.com1_tid = WhoIs(COM1_REG_NAME);
	train_global.com2_tid = WhoIs(COM2_REG_NAME);
	train_global.track_nodes = track_nodes;
	train_global.switch_table = switch_table;
	train_global.train_data = train_data;
	train_global.track_reservation = track_reservation;

	/* initial UI */
	iprintf(train_global.com2_tid, 10, "\e[2J");
	iprintf(train_global.com2_tid, 30, "\e[1;1HTime: 00:00.0");
	iprintf(train_global.com2_tid, 20, "\e[2;1HCommand:");
	int i;
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHSwitch Table:", 5, 1);
	iprintf(train_global.com2_tid, 10, "\e[%d;%dH", 6, 2);
	for (i = 1; i < 10; i++) {
		iprintf(train_global.com2_tid, 10, "00%d:C ", i);
	}

	iprintf(train_global.com2_tid, 10, "\e[%d;%dH", 7, 2);
	for (i = 10; i < 19; i++) {
		iprintf(train_global.com2_tid, 10, "0%d:C ", i);
	}

	iprintf(train_global.com2_tid, 10, "\e[%d;%dH", 8, 2);
	for (i = 153; i < 157; i++) {
		iprintf(train_global.com2_tid, 10, "%d:C ", i);
	}

	iprintf(train_global.com2_tid, 30, "\e[%d;%dHTrain Position:", 10, 2);

	iprintf(train_global.com2_tid, 30, "\e[%d;%dHCurrent Location:", 11, 2);
	// current node name 11;20
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHHas Gone:\e[%d;%dHmm", 11, 30, 11, 46);
	// dist 11;40
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHNext Location:", 12, 2);
	// predict node name 12;17
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHRemain:\e[%d;%dHmm", 12, 30, 12, 44);
	// dist 12;38
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHVelocity: 0", 14, 2);
	// velocity 14;12
	iprintf(train_global.com2_tid, 30, "\e[%d;%dHDestination: ", 15, 2);
	// Destination node 15;15

	iprintf(train_global.com2_tid, 30, "\e[%d;%dHPred - Real: ", 17, 2);
	// Dist diff : 17;15

	iprintf(train_global.com2_tid, 10, "\e[%d;%dH\e[s", 3, 1);

	Create(8, trainclockserver);
	tid = CreateWithArgs(8, trainCenter, (int)(&train_global), 0, 0, 0);
	train_global.center_tid = tid;

	Receive(&tid, NULL, 0);

	assert(0, "Train Bootstrap exit");
}
