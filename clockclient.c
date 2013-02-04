#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

typedef struct cs_client_reply {
	int time_tick;
	int delay_num;
} CSClientReply;

void timeclient() {
	CSClientReply reply;
	char send_msg[2] = "s";
	Send(0, send_msg, 2, (char*)(&reply), sizeof(CSClientReply));
	int tick = reply.time_tick;
	int counter = 0;
	int delay_num = reply.delay_num;
	int myTid = MyTid();
	while (counter < delay_num) {
		counter++;
		Delay(tick);
		// DEBUG(DB_CS, "| CS:\tdelayed ticks: %d\n", tick);
		bwprintf(COM2, "%d: %d, %d\n", myTid, tick, counter);
	}
}


void umain() {
	int tid;
	tid = Create(4, nameserver);
	bwprintf(COM2, "Created: %d Nameserver\n", tid);
	tid = Create(2, clockserver);
	bwprintf(COM2, "Created: %d Clockserver\n", tid);
	tid = Create(5, timeclient);
	bwprintf(COM2, "Created: %d ClientP3\n", tid);
	tid = Create(6, timeclient);
	bwprintf(COM2, "Created: %d ClientP4\n", tid);
	tid = Create(7, timeclient);
	bwprintf(COM2, "Created: %d ClientP5\n", tid);
	tid = Create(8, timeclient);
	bwprintf(COM2, "Created: %d ClientP6\n", tid);
	createIdleTask();
	CSClientReply reply;
	reply.time_tick = 10;
	reply.delay_num = 20;
	Receive(&tid, NULL, 0);
	Reply(tid, (char*) (&reply), sizeof(CSClientReply));
	reply.time_tick = 23;
	reply.delay_num = 9;
	Receive(&tid, NULL, 0);
	Reply(tid, (char*) (&reply), sizeof(CSClientReply));
	reply.time_tick = 33;
	reply.delay_num = 6;
	Receive(&tid, NULL, 0);
	Reply(tid, (char*) (&reply), sizeof(CSClientReply));
	reply.time_tick = 71;
	reply.delay_num = 3;
	Receive(&tid, NULL, 0);
	Reply(tid, (char*) (&reply), sizeof(CSClientReply));
}


