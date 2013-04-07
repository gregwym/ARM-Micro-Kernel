#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

inline void printLineDivider(int com2_tid) {
	iprintf(com2_tid, BUFFER_LEN, "--------------------------------------------------------------------------------\n");
}

void initializeTrainUI(int com2_tid, TrainData *train_data) {
	iprintf(com2_tid, BUFFER_LEN, "-----TRAIN #%d------------------------------------------------------------------\n", train_data->id);

	iprintf(com2_tid, BUFFER_LEN, "Current       |               | ");
	// current node name 11;20
	iprintf(com2_tid, BUFFER_LEN, "Has Gone      |               | \n");
	// dist 11;40
	iprintf(com2_tid, BUFFER_LEN, "Next          |               | ");
	// predict node name 12;17
	iprintf(com2_tid, BUFFER_LEN, "Remain        |               | \n");
	// dist 12;38
	iprintf(com2_tid, BUFFER_LEN, "Velocity      |               | ");
	// velocity 14;12
	iprintf(com2_tid, BUFFER_LEN, "Pred - Real   |               | \n");
	// Dist diff : 17;15
	iprintf(com2_tid, BUFFER_LEN, "Destination   |               | \n");
	// Destination node 15;15
}

void initializeUI(int com2_tid, TrainData *trains_data) {
	int i;
	int switch_ids[SWITCH_TOTAL];
	for(i = 0; i < SWITCH_TOTAL; i++) {
		switch_ids[i] = i < SWITCH_NAMING_MAX ? i + SWITCH_NAMING_BASE : i + SWITCH_NAMING_MID_BASE - SWITCH_NAMING_MAX;
	}

	// Clear the screen
	iprintf(com2_tid, BUFFER_LEN, "\e[2J\e[1;1H");

	iprintf(com2_tid, BUFFER_LEN, "Märklin Digital Train Control Panel                  Time elapsed:   00:00:0\n");
	printLineDivider(com2_tid);
	iprintf(com2_tid, BUFFER_LEN, "Last Command  | \n");
	printLineDivider(com2_tid);
	iprintf(com2_tid, BUFFER_LEN, "Track Switchs | ");

	int cells = (WIDTH_SWITCH_TABLE * HEIGHT_SWITCH_TABLE - 1);
	for(i = 0; i < cells; i++) {
		int index = (i % WIDTH_SWITCH_TABLE) * HEIGHT_SWITCH_TABLE + i / WIDTH_SWITCH_TABLE;
		if(index < SWITCH_TOTAL) {
			int id = switch_ids[index];
			iprintf(com2_tid, BUFFER_LEN, "%d   ", id);
			if(id / 100 == 0) Putc(com2_tid, ' ');
			if(id / 10 == 0) Putc(com2_tid, ' ');
			iprintf(com2_tid, BUFFER_LEN, "| C     | ");
		}
		if(i % WIDTH_SWITCH_TABLE == (WIDTH_SWITCH_TABLE - 1)) iprintf(com2_tid, BUFFER_LEN, "\n              | ");
	}
	Putc(com2_tid, '\n');
	for(i = 0; i < TRAIN_MAX; i++) {
		initializeTrainUI(com2_tid, &(trains_data[i]));
	}
	printLineDivider(com2_tid);

	iprintf(com2_tid, 5, "\e[s");
}

void trainBootstrap() {

	int i, tid;

	/* Track Data */
	track_node track_nodes[TRACK_MAX];
	init_trackb(track_nodes);

	/* Switch Data */
	char switch_table[SWITCH_TOTAL];
	memset(switch_table, SWITCH_CUR, SWITCH_TOTAL);

	/* Orbit Data */
	Orbit orbits[ORBIT_MAX];
	LinkedList satellite_lists[ORBIT_MAX];
	init_orbit1(&(orbits[0]), track_nodes);
	init_orbit2(&(orbits[1]), track_nodes);
	init_orbit3(&(orbits[2]), track_nodes);
	// init_orbit4(&(orbits[3]), track_nodes);
	
	for (i = 0; i < ORBIT_MAX; i++) {
		orbits[i].satellite_list = &(satellite_lists[i]);
	}

	/* Satellite Nodes */
	LinkedListNode satellite_nodes[TRAIN_MAX];

	/* Train Data */
	TrainData trains_data[TRAIN_MAX];
	TrainData *train_id_data[TRAIN_ID_MAX];
	init_train49(&(trains_data[0]));
	init_train50(&(trains_data[1]));
	init_train51(&(trains_data[2]));

	for(i = 0; i < TRAIN_MAX; i++) {
		trains_data[i].index = i;
		train_id_data[trains_data[i].id] = &(trains_data[i]);
		satellite_nodes[i].current = &(trains_data[i]);
		satellite_nodes[i].previous = NULL;
		satellite_nodes[i].next = NULL;
	}

	/* Track Reservation */
	TrainData *track_reservation[TRACK_MAX];
	TrainData *recovery_reservation[TRACK_MAX];
	memset(track_reservation, (int)NULL, sizeof(TrainData *) * TRACK_MAX);
	memset(recovery_reservation, (int)NULL, sizeof(TrainData *) * TRACK_MAX);

	/* Train Global */
	TrainGlobal train_global;
	train_global.com1_tid = WhoIs(COM1_REG_NAME);
	train_global.com2_tid = WhoIs(COM2_REG_NAME);
	train_global.track_nodes = track_nodes;
	train_global.switch_table = switch_table;
	train_global.trains_data = trains_data;
	train_global.train_id_data = train_id_data;
	train_global.orbits = orbits;
	train_global.satellite_nodes = satellite_nodes;
	train_global.track_reservation = track_reservation;
	train_global.recovery_reservation = recovery_reservation;
	
	/* initial map */
	train_global.map[0] = &(track_nodes[14]);		//A15
	train_global.map[1] = &(track_nodes[52]);		//D5
	train_global.map[2] = &(track_nodes[3]);		//A4
	train_global.map[3] = &(track_nodes[55]);		//D8
	train_global.map[4] = &(track_nodes[52]);		//D5
	train_global.map[5] = &(track_nodes[43]);		//C12
	train_global.map[6] = &(track_nodes[74]);		//E11
	train_global.map[7] = &(track_nodes[77]);		//E14
	train_global.map[8] = &(track_nodes[3]);		//A4

	/* initial UI */
	initializeUI(train_global.com2_tid, trains_data);

	Create(8, trainclockserver);
	tid = CreateWithArgs(8, trainCenter, (int)(&train_global), 0, 0, 0);
	train_global.center_tid = tid;
	// tid = CreateWithArgs(8, routeServer, (int)(&train_global), 0, 0, 0);
	// train_global.route_server_tid = tid;

	Receive(&tid, NULL, 0);

	assert(0, "Train Bootstrap exit");
}
