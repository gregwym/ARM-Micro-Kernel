#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>
#include <usertrap.h>
#include <syscall.h>

void tlistInitial(TaskList *tlist, Task **heads, Task **tails) {
	tlist->curtask = NULL;
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

void tarrayInitial(Task *task_array, char *stacks) {
	int i;
	for (i = 0; i < TASK_MAX; i++) {
		task_array[i].tid = i - TASK_MAX;
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

int insertTask(TaskList *tlist, Task *new_task) {
	//assert(tlist->list_counter < tlist->list_size, "Exceed tlist size!");
	//assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	int i;
	int priority = new_task->priority;

	// Find and fill in new task
	// If it is the first task
	if (tlist->head == NULL) {
		tlist->head = new_task;
		tlist->priority_heads[priority] = new_task;
		tlist->priority_tails[priority] = new_task;
	}
	// If it is the new highest priority
	else if (tlist->head->priority > priority) {
		tlist->head = new_task;
		tlist->priority_heads[priority] = new_task;
		tlist->priority_tails[priority] = new_task;

		// Link tail
		for (i = priority + 1; i < TASK_PRIORITY_MAX; i++) {
			if (tlist->priority_heads[i] != NULL) {
				new_task->next = tlist->priority_heads[i];
				break;
			}
		}
	}
	// If it is not new highest, but new priority
	else if (tlist->priority_tails[priority] == NULL) {
		tlist->priority_heads[priority] = new_task;
		tlist->priority_tails[priority] = new_task;
		// Link higher priority task next to new_task
		for (i = priority - 1; i >= 0; i--) {
			if (tlist->priority_tails[i] != NULL) {
				new_task->next = tlist->priority_tails[i]->next;
				tlist->priority_tails[i]->next = new_task;
				break;
			}
		}
		// assert(i >= 0, "TaskList: Failed to find higher priority task");
	}
	// If not new priority
	else {
		new_task->next = tlist->priority_tails[priority]->next;
		tlist->priority_tails[priority]->next = new_task;
		tlist->priority_tails[priority] = new_task;
	}

	DEBUG(DB_TASK, "Task(tid: %d) is created, stack at %x\n", new_task->tid, new_task->init_sp);
	return 1;
}

Task* removeCurrentTask(TaskList *tlist, FreeList *flist) {
	// assert(tlist->head != NULL, "TaskList: list is empty");
	int top_priority = tlist->head->priority;
	Task *ret = NULL;

	// Adjust top_priority head and tails
	if (tlist->priority_heads[top_priority] == tlist->priority_tails[top_priority]) {
		tlist->priority_heads[top_priority] = NULL;
		tlist->priority_tails[top_priority] = NULL;
	} else {
		tlist->priority_heads[top_priority] = tlist->priority_heads[top_priority]->next;
	}
	
	ret = tlist->head;
	
	if (tlist->head == tlist->curtask) {
		tlist->head = tlist->head->next;
	}

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

Task *createTask(FreeList *flist, int priority, void * context()) {
	//assert(flist->head != NULL, "Task array is already full!");
	Task *ret;
	ret = flist->head;
	//assert(ret->state == Empty, "Invalid task space to use!");
	ret->tid = ret->tid + TASK_MAX;
	ret->generation += 1;
	ret->state = Ready;
	ret->priority = priority;

	flist->head = flist->head->next;

	// Link to next ready task and parent tid should be set when pushing to task list
	ret->next = NULL;
	ret->parent_tid = -1;

	ret->resume_point = context;
	ret->current_sp = initTrap(ret->init_sp, DATA_REGION_BASE + Exit);

	return ret;
}

void moveCurrentTaskToEnd(TaskList *tlist) {
	int priority = tlist->curtask->priority;
	
	if (tlist->priority_heads[priority] != tlist->priority_tails[priority]) {
		if (tlist->head->priority < priority) {
			tlist->head->next = tlist->curtask->next;
		} else {
			tlist->head = tlist->curtask->next;
		}
		tlist->priority_heads[priority] = tlist->curtask->next;
		tlist->curtask->next = tlist->priority_tails[priority]->next;
		tlist->priority_tails[priority]->next = tlist->curtask;
		tlist->priority_tails[priority] = tlist->curtask;
	}
}

void refreshCurtask(TaskList *tlist) {
	tlist->curtask = tlist->head;
}
