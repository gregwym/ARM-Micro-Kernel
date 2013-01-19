
#include <bwio.h>
#include <ts7200.h>
#include <task.h>
#include <kernel.h>
#include <types.h>

int main( int argc, char* argv[] ) {
	
	// char str[] = "Hello\n\r";
	bwsetfifo( COM2, OFF );
	
	char *data;
	
	TaskList tlist;
	Task tasks[TASKNUM];
	char *stack[TASKNUM * TASKSIZE];
	Task *priority_head[PRIORITY_LVL];
	Task *priority_tail[PRIORITY_LVL];
	
	tlistInitial(&tlist, tasks, TASKNUM, priority_head, priority_tail, PRIORITY_LVL, stack);
	
	tlistPush(&tlist, data, 6);		//0
	tlistPush(&tlist, data, 4);		//1
	tlistPush(&tlist, data, 2);		//2
	tlistPush(&tlist, data, 2);		//3
	tlistPush(&tlist, data, 4);		//4
	tlistPush(&tlist, data, 6);		//5
	//2, 3, 1, 4, 0, 5
	
	Task *tmp = tlist.head;
	
	while (tmp != NULL) {
		bwprintf(COM2, "%d\n", tmp->tid);
		tmp = tmp->next;
	}
	
	tlistPop(&tlist);
	tlistPop(&tlist);
	tlistPop(&tlist);
	
	tmp = tlist.head;
	
	while (tmp != NULL) {
		bwprintf(COM2, "after: %d\n", tmp->tid);
		tmp = tmp->next;
	}
	
	//4, 0, 5
	
	
	return 0;
}

