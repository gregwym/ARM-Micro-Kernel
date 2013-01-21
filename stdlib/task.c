#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>


int tlistInitial (TaskList *tlist, Task *task_array, Task **heads, Task **tails, char* stacks) {
	tlist->list_counter = 0;
	tlist->task_array = task_array;
	tlist->head = NULL;
	tlist->priority_heads = heads;
	tlist->priority_tails = tails;
	
	int i;
	
	for (i = 0; i < TASK_MAX; i++) {
		task_array[i].tid = -1;
		task_array[i].state = Empty;
		task_array[i].priority = -1;
		task_array[i].init_sp = &(stacks[(i+1) * TASK_STACK_SIZE - 4]);
		task_array[i].next = NULL;
		task_array[i].parent_tid = -1;
	}
	
	// TODO: Add magic number to the end of each stack, so can detect stack overflow

	for (i = 0; i < TASK_MAX; i++) {
		heads[i] = NULL;
		tails[i] = NULL;
	}

	return 0;
}

int tlistPush (TaskList *tlist, void *context, int priority) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	int i;
	Task *new_task;
	
	// Find and fill in new task
	new_task = &(tlist->task_array[tlist->list_counter]);
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
	if (tlist->priority_tails[priority] == NULL) {
		tlist->priority_heads[priority] = new_task;
		tlist->priority_tails[priority] = new_task;
		// Link higher priority task next to new_task
		for (i = priority - 1; i >= 0; i--) {
			if (tlist->priority_tails[i] != NULL) {
				tlist->priority_tails[i]->next = new_task;
				break;
			}
		}
	} else {
		tlist->priority_tails[priority]->next = new_task;
		tlist->priority_tails[priority] = new_task;
	}
	
	//link to next p_head
	for (i = priority + 1; i < TASK_PRIORITY_MAX; i++) {
		if (tlist->priority_heads[i] != NULL) {
			new_task->next = tlist->priority_heads[i];
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
	if (tlist->priority_heads[top_priority] == tlist->priority_tails[top_priority]) {
		tlist->priority_heads[top_priority] = NULL;
		tlist->priority_tails[top_priority] = NULL;
	} else {
		tlist->priority_heads[top_priority] = tlist->priority_heads[top_priority]->next;
	}
	
	ret = tlist->head;
	tlist->head = tlist->head->next;
	return ret;
}

	