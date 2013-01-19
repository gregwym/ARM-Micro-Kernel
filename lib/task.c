#include "types.h"
#include "task.h"
#include "kernel.h"

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
		tasks[i].tid = i;
		//tasks[i]->parent
		//tasts[i]->state
		tasks[i]->position = stack[i* STACKSIZE];
		tasks[i]->tf[13] = (char) position;
		//tasks[i]->tf
		
	}
	
	for (i = 0; i < max_p; i++) {
		head[i] = NULL;
		tail[i] = NULL;
	}
}

int tlistPush (TaskList *tlist, void *context, int priority) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= PRIORITY_LVL, "Invalid priority value!");
	int i;
	Task *new_task;
	
	new_task = &tlist->tasklist[tlist->list_counter];
	new_task->tf[14] = context;
	new_task->priority = priority;
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
	for (i = priority + 1; i < PRIORITY_LVL; i++) {
		if (tlist->priority_head[i] != NULL) {
			new_task->next = tlist->priority_head[i];
			break;
		}
	}
	
	tlist->list_counter++;
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
	tlist->head = tlist->head->next;
	return ret;
}

	