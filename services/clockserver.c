#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

#define CS_TID						2
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

typedef struct time_reply {
	unsigned int	time;
} TimeReply;

typedef union {
	char				type;
	DelayQuery			delayQuery;
	DelayUntilQuery		delayUntilQuery;
	TimeQuery			timeQuery;
} ClockServerMsg;

void notifier() {
	char send[1] = "";
	while (1) {
		AwaitEvent(EVENT_TIME_ELAP, NULL, 0);
		Send(CS_TID, send, 1, NULL, 0);
	}
}

int Delay( int ticks ) {
	assert(ticks >= 0, "Delay ticks is negative");
	DelayQuery query;
	query.type = CS_QUERY_TYPE_DELAY;
	query.delay_tick = (unsigned int) ticks;
	return Send(CS_TID, (char*)(&query), sizeof(DelayQuery), NULL, 0);
}

int DelayUntil( int ticks ) {
	assert(ticks >= 0, "DelayUntil ticks is negative");
	DelayUntilQuery query;
	query.type = CS_QUERY_TYPE_DELAY_UNTIL;
	query.delay_time = (unsigned int) ticks;
	return Send(CS_TID, (char*)(&query), sizeof(DelayUntilQuery), NULL, 0);
}
	
int Time() {
	int ret = -1;
	TimeQuery query;
	TimeReply reply;
	query.type = CS_QUERY_TYPE_TIME;
	ret = Send(CS_TID, (char*)(&query), sizeof(TimeQuery), (char*)(&reply), sizeof(TimeReply));
	if (ret < 0) return ret;
	return reply.time;
}

void clockserver() {
	unsigned int time = 0;
	TimeReply reply;
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
	int notifier_tid = Create(1, notifier);
	
	while (1) {
		Receive(&tid, (char *)&message, sizeof(ClockServerMsg));
		if (tid == notifier_tid) {
			Reply(tid, NULL, 0);
			time++;
			DEBUG(DB_CS, "| CS:\tTime : %d\n", time);
			while (minheap.heapsize > 0 && time >= minheap.data[0]->key) {
				HeapNode *top = minHeapPop(&minheap);
				Reply(*(int *)(top->datum), NULL, 0);
			}
			continue;
		}

		switch(message.type) {
			case CS_QUERY_TYPE_DELAY:
				// delay
				tid_array[tid % TASK_MAX] = tid;
				nodes[tid % TASK_MAX].key = message.delayQuery.delay_tick + time;
				nodes[tid % TASK_MAX].datum = &(tid_array[tid % TASK_MAX]);
				minHeapInsert(&minheap, &(nodes[tid % TASK_MAX]));
				break;
			case CS_QUERY_TYPE_DELAY_UNTIL:
				// delay until
				if (message.delayUntilQuery.delay_time <= time) {
					Reply(tid, NULL, 0);
				} else {
					tid_array[tid % TASK_MAX] = tid;
					nodes[tid % TASK_MAX].key = message.delayUntilQuery.delay_time;
					nodes[tid % TASK_MAX].datum = &(tid_array[tid % TASK_MAX]);
					minHeapInsert(&minheap, &(nodes[tid % TASK_MAX]));
				}
				break;
			case CS_QUERY_TYPE_TIME:
				// time
				reply.time = time;
				Reply(tid, (char*)(&reply), sizeof(TimeReply));
				break;
			default:
				assert(0, "Clockserver received unknown query type");
				break;
		}
	}
}
