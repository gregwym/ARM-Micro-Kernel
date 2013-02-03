#include <services.h>
#include <klib.h>
#include <unistd.h>

#define CS_QUERY_TYPE_DELAY 		1
#define CS_QUERY_TYPE_DELAY_UNTIL 	2
#define CS_QUERY_TYPE_TIME			3

typedef struct delay_query {
	char 			type;
	unsigned int  	delay_tick;
} DelayQuery;

typedef struct delay_until_query {
	char 			type;
	unsigned int  	delay_time;
} DelayUntilQuery;

typedef struct time_query {
	char 			type;
} TimeQuery;

typedef struct reply_query {
	unsigned int	time;
} ReplyQuery;

typedef union {
	char				type;
	DelayQuery			delayQuery;
	DelayUntilQuery		delayUntilQuery;
	TimeQuery			timeQuery;
} ClockServerMsg;

void notifier() {
	char msg[3] = "ts";;
	char send[2] = "w";
	char reply[1];
	int time_server_tid = WhoIs(msg);
	while (1) {
		AwaitEvent(EVENT_TIME_ELAP, NULL, 0);
		Send(time_server_tid, send, 1, reply, 1);
	}
}

void timeclient_p3() {
	DelayQuery query;
	query.type = CS_QUERY_TYPE_DELAY;
	query.delay_tick = 10;
	char msg[3] = "ts";;
	char reply[1];
	int time_server_tid = WhoIs(msg);
	int counter = 0;
	while (counter < 20) {
		Send(time_server_tid, (char*)(&query), sizeof(DelayQuery), reply, 1);
		counter++;
	}
}

void timeclient_p4() {
	DelayQuery query;
	query.type = CS_QUERY_TYPE_DELAY;
	query.delay_tick = 23;
	char msg[3] = "ts";;
	char reply[1];
	int time_server_tid = WhoIs(msg);
	int counter = 0;
	while (counter < 9) {
		Send(time_server_tid, (char*)(&query), sizeof(DelayQuery), reply, 1);
		counter++;
	}
}

void timeclient_p5() {
	DelayQuery query;
	query.type = CS_QUERY_TYPE_DELAY;
	query.delay_tick = 33;
	char msg[3] = "ts";;
	char reply[1];
	int time_server_tid = WhoIs(msg);
	int counter = 0;
	while (counter < 6) {
		Send(time_server_tid, (char*)(&query), sizeof(DelayQuery), reply, 1);
		counter++;
	}
}

void timeclient_p6() {
	DelayQuery query;
	query.type = CS_QUERY_TYPE_DELAY;
	query.delay_tick = 71;
	char msg[3] = "ts";;
	char reply[1];
	int time_server_tid = WhoIs(msg);
	int counter = 0;
	while (counter < 3) {
		Send(time_server_tid, (char*)(&query), sizeof(DelayQuery), reply, 1);
		counter++;
	}
}

void clockserver() {
	unsigned int time = 0;
	char msg[3] = "ts";
	ReplyQuery reply;
	RegisterAs(msg);
	int notifier_tid = Create(3, notifier);
	ClockServerMsg message;
	int tid;
	
	/* heap implement */
	Heap minheap;
	HeapNode *heap_data[TASK_MAX];
	heapInitial(&minheap, heap_data, TASK_MAX);
	HeapNode nodes[TASK_MAX];
	heapNodesInitial(nodes, TASK_MAX);
	int tid_array[TASK_MAX]; 	//actual datum in HeapNode
	int i = -1;
	for (i = 0; i < TASK_MAX; i++) {
		tid_array[i] = -1;
	}
	
	while (1) {
		Receive(&tid, (char *)&message, sizeof(ClockServerMsg));
		if (tid == notifier_tid) {
			Reply(tid, NULL, 0);
			time++;
			if (minheap.heapsize > 0 && time > *(int *)(minheap.data[0]->datum)) {
				HeapNode *top = minHeapPop(&minheap);
				Reply(*(int *)(top->datum), NULL, 0);
			}	
		} else {
			switch(message.type) {
				case 1:
					// delay
					tid_array[tid % TASK_MAX] = tid;
					nodes[tid % TASK_MAX].key = message.delayQuery.delay_tick + time;
					nodes[tid % TASK_MAX].datum = &(tid_array[tid % TASK_MAX]);
					minHeapInsert(&minheap, &(nodes[tid % TASK_MAX]));
					break;
				case 2:
					// delay until
					tid_array[tid % TASK_MAX] = tid;
					nodes[tid % TASK_MAX].key = message.delayQuery.delay_tick;
					nodes[tid % TASK_MAX].datum = &(tid_array[tid % TASK_MAX]);
					minHeapInsert(&minheap, &(nodes[tid % TASK_MAX]));
					break;
				case 3:
					// time
					reply.time = time;
					Reply(tid, (char*)(&reply), sizeof(ReplyQuery));
					break;
				default:
					break;
					
			}
		}
	}
}
