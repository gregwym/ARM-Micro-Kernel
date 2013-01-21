#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>


int tlistInitial (TaskList *tlist, Task *task_array, Task **head, Task **tail, char* stacks) {}
	tlist->list_counter = 0;
	tlist->task_array = task_array;
	tlist->head = NULL;
	tlist->priority_heads = heads;
	tlist->priority_tails = tails;
	
	int i;
	
	for (i = 0; i < TASK_MAX; i++) {
		tasks[i].tid = -1;
		tasts[i].state = Empty;
		tasks[i].priority = -1;
		tasks[i].init_sp = &(stacks[(i+1) * TASK_STACK_SIZE - 4]);
		tasks[i].next = NULL;
		tasks[i].parent_tid = -1;
	}
	
	// TODO: Add magic number to the end of each stack, so can detect stack overflow

	for (i = 0; i < TASK_MAX; i++) {
		head[i] = NULL;
		tail[i] = NULL;
	}
}

int tlistPush (TaskList *tlist, void *context, int priority) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	int i;
	Task *new_task;
	
	// Find and fill in new task
	new_task = &(tlist->tasklist[tlist->list_counter]);
	// TODO: Save trapframe on user stack (maybe not here)
	new_task->tid = tlist->list_counter;
	new_task->state = Ready;
	new_task->priority = priority;
	
	// Change head
	if (tlist->head == NULL) {
		tlist->head = new_task;
	} else if (tlist->head->priority > priority) {
		tlist->head = new_task;
	}
	
	//change tail's link
	if (tlist->priority_tail[priority] == NULL) {
		tlist->priority_head[priority] = new_task;
		tlist->priority_tail[priority] = new_task;
		// Link higher priority task next to new_task
		for (i = priority - 1; i >= 0; i--) {
			if (tlist->priority_tail[i] != NULL) {
				tlist->priority_tail[i]->next = new_task;
				break;
			}
		}
	} else {
		tlist->priority_tail[priority]->next = new_task;
		tlist->priority_tail[priority] = new_task;
	}
	
	//link to next p_head
	for (i = priority + 1; i < TASK_PRIORITY_MAX; i++) {
		if (tlist->priority_head[i] != NULL) {
			new_task->next = tlist->priority_head[i];
			break;
		}
	}
	
	tlist->list_counter++;
	
	// bwprintf("Task(tid: %d) is created at %x\n", new_task->tid, new_task->position);
	return 1;
}

Task* tlistPop(TaskList *tlist) {
	//assert(tlist->head != NULL, "TaskList is already empty!");
	int top_priority = -1;
	Task *ret = NULL;
	
	top_priority = tlist->head->priority;
	
	// Adjust top_priority head and tails
	if (tlist->priority_head[top_priority] == tlist->priority_tail[top_priority]) {
		tlist->priority_head[top_priority] = NULL;
		tlist->priority_tail[top_priority] = NULL;
	} else {
		tlist->priority_head[toppriority] = tlist->priority_head[toppriority]->next;
	}
	
	ret = tlist->head;
	tlist->head = tlist->head->next;
	return ret;
}

	