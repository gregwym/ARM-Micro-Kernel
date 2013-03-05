#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

void trainBootstrap() {

	/* initial UI */

	/* Temporary disable the UI
	iprintf("\e[2J");
	iprintf("\e[1;1HTime: 00:00.0");
	iprintf("\e[2;1HCommand:");
	int i;
	iprintf("\e[%d;%dHSwitch Table:", 5, 1);
	iprintf("\e[%d;%dH", 6, 2);
	for (i = 1; i < 10; i++) {
		iprintf("00%d:? ", i);
	}

	iprintf("\e[%d;%dH", 7, 2);
	for (i = 10; i < 19; i++) {
		iprintf("0%d:? ", i);
	}

	iprintf("\e[%d;%dH", 8, 2);
	for (i = 153; i < 157; i++) {
		iprintf("%d:? ", i);
	}
	iprintf("\e[9;1HSensor Data:");
	iprintf("\e[%d;%dH", 3, 1);
	*/

	/* Track Data */
	track_node track_nodes[TRACK_MAX];
	init_trackb(track_nodes);

	/* Switch Data */
	char switch_table[SWITCH_TOTAL];
	memset(switch_table, SWITCH_CUR, SWITCH_TOTAL);

	/* Train Data */
	TrainData train_data[TRAIN_MAX];
	init_train37(&(train_data[0]));

	/* Train Global */
	TrainGlobal train_global;
	train_global.com1_tid = WhoIs(COM1_REG_NAME);
	train_global.com2_tid = WhoIs(COM2_REG_NAME);
	train_global.track_nodes = track_nodes;
	train_global.switch_table = switch_table;
	train_global.train_data = train_data;

	// Create(8, trainclockserver);
	CreateWithArgs(8, trainCenter, (int)(&train_global), 0, 0, 0);

	int tid;
	Receive(&tid, NULL, 0);

	assert(0, "Train Bootstrap exit");
}
