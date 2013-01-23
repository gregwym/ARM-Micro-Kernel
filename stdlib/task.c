#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>


void tlistInitial (TaskList *tlist, Task **heads, Task **tails) {
	tlist->head = NULL;
	tlist->priority_heads = heads;
	tlist->priority_tails = tails;
	
	int i;
	
	// TODO: Add magic number to the end of each stack, so can detect stack overflow

	for (i = 0; i < TASK_MAX; i++) {
		heads[i] = NULL;
		tails[i] = NULL;
	}

}	
	
int tlistPush (TaskList *tlist, Task *new_task) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	int i;
	int priority = new_task->priority;
	
	// Find and fill in new task
	// Change head
	new_task->next = NULL;
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
	
	
	// bwprintf("Task(tid: %d) is created at %x\n", new_task->tid, new_task->position);
	return 1;
}

Task* popTask(TaskList *tlist, FreeList *flist) {
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
	
	if (flist->head == NULL) {
		flist->head = ret;
		flist->tail = ret;
		ret->next = NULL;
		ret->state = Empty;
	} else {
		flist->tail->next = ret;
		flist->tail = ret;
		flist->tail->next = NULL;
		flist->tail->state = Empty;
	}
	
	return ret;
}

void tarrayInitial(Task *task_array, char *stacks) {
	int i;
	for (i = 0; i < TASK_MAX; i++) {
		task_array[i].tid = i;
		task_array[i].generation = 0;
		task_array[i].init_sp = &(stacks[(i+1) * TASK_STACK_SIZE - 4]);
	}
}

void flistInitial(FreeList *flist, Task *task_array) {
	int i;
	for (i = 0; i < TASK_MAX - 1; i++) {
		task_array[i].next = &(task_array[i+1]);
		task_array[i].state = Empty;
	}
	task_array[TASK_MAX - 1].next = NULL;
	task_array[TASK_MAX - 1].state = Empty;
	
	flist->head = &task_array[0];
	flist->tail = &task_array[TASK_MAX - 1];
}

Task *createTask(FreeList *flist, int priority, void * context()) {
	//assert(flist->head != NULL, "Task array is already full!");
	Task *ret;
	ret = flist->head;
	//assert(ret->state == Empty, "Invalid task space to use!");
	ret->tid = ret->tid + TASK_MAX * ret->generation;
	ret->generation += 1;
	ret->priority = priority;
	flist->head = flist->head->next;
	
	return ret;
}

