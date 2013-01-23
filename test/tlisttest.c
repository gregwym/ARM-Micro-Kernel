
#include <bwio.h>
#include <ts7200.h>
#include <task.h>
#include <kernel.h>
#include <types.h>

int main( int argc, char* argv[] ) {
	
	bwsetfifo( COM2, OFF );
	
	int i;
	
	void * prt;
	TaskList tlist;
	FreeList flist;
	Task task_array[TASK_MAX];
	char stack[TASK_MAX * TASK_STACK_SIZE];
	Task *priority_head[TASK_PRIORITY_MAX];
	Task *priority_tail[TASK_PRIORITY_MAX];
	
	tarrayInitial(&task_array, &stack);
	flistInitial(&flist, &task_array);
	tlistInitial(&tlist, priority_head, priority_tail);
	
	Task *tmp;
	
	for (i = 0; i < 10; i++) {
		tlistPush(&tlist, createTask(&flist, i, prt));
	}
	
	tmp = tlist.head;
	
	while (tmp != NULL) {
		bwprintf(COM2, "Insert: %d\n", tmp->tid);
		tmp = tmp->next;
	}
	
	for (i = 0; i < 10; i++) {
		popTask(&tlist, &flist);
	}
	
	while (tmp != NULL) {
		bwprintf(COM2, "Pop: %d\n", tmp->tid);
		tmp = tmp->next;
	}
	
	for (i = 0; i < 10; i++) {
		tlistPush(&tlist, createTask(&flist, 9 - i, prt));
	}
	
	tmp = tlist.head;
	
	while (tmp != NULL) {
		bwprintf(COM2, "Insert: %d\n", tmp->tid);
		tmp = tmp->next;
	}
	
	
	return 0;
}

