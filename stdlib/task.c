#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>


int tlistInitial (TaskList *tlist, Task *tasks, int tlist_size, Task **head, Task **tail, int max_p, char* stack) {
	tlist->list_size = tlist_size;
	tlist->list_counter = 0;
	tlist->max_plvl = max_p;
	tlist->tasklist = tasks;
	tlist->head = NULL;
	tlist->priority_head = head;
	tlist->priority_tail = tail;
	
	int i;
	
	for (i = 0; i < tlist_size; i++) {
		tasks[i].next = NULL;
		tasks[i].tid = -1;
		//tasks[i]->parent
		//tasts[i]->state
		tasks[i].position = &stack[(i+1) * TASK_STACK_SIZE - 4];
		//tasks[i]->tf
		
	}
	
	for (i = 0; i < max_p; i++) {
		head[i] = NULL;
		tail[i] = NULL;
	}
}

int tlistPush (TaskList *tlist, void *context, int priority) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	int i;
	Task *new_task;
	
	new_task = &tlist->tasklist[tlist->list_counter];
	new_task->tf[15] = (int)context;
	new_task->priority = priority;
	new_task->state = Ready;
	new_task->tid = tlist->list_counter;
	//todo, initialisze Task
	
	
	//change head
	if (tlist->list_counter == 0) {
		tlist->head = new_task;
	} else if (tlist->head->priority > priority) {
		tlist->head = new_task;
	}
	
	//change tail's link
	if (tlist->priority_tail[priority] == NULL) {
		tlist->priority_head[priority] = new_task;
		tlist->priority_tail[priority] = new_task;
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
	int toppriority;
	Task *ret;
	
	toppriority = tlist->head->priority;
	
	if (tlist->priority_head[toppriority] == tlist->priority_tail[toppriority]) {
		tlist->priority_head[toppriority] = NULL;
		tlist->priority_tail[toppriority] = NULL;
	} else {
		tlist->priority_head[toppriority] = tlist->priority_head[toppriority]->next;
	}
	
	ret = tlist->head;
	ret->state = Zombie;
	tlist->head = tlist->head->next;
	return ret;
}

	