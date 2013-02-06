#include <task.h>
#include <klib.h>
#include <unistd.h>
#include <kern/md_const.h>
#include <intern/trapframe.h>

void tlistInitial(TaskList *task_list, Task **heads, Task **tails) {
	task_list->curtask = NULL;
	task_list->head = NULL;
	task_list->priority_heads = heads;
	task_list->priority_tails = tails;

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
		task_array[i].state = Zombie;
	}
}

void flistInitial(FreeList *free_list, Task *task_array) {
	int i;
	for (i = 0; i < TASK_MAX - 1; i++) {
		task_array[i].next = &(task_array[i+1]);
	}
	task_array[TASK_MAX - 1].next = NULL;

	free_list->head = &task_array[0];
	free_list->tail = &task_array[TASK_MAX - 1];
}

int insertTask(TaskList *task_list, Task *new_task) {
	int i;
	int priority = new_task->priority;

	// Change the task state to Ready
	new_task->state = Ready;

	// Find and fill in new task
	// If it is the first task
	if (task_list->head == NULL) {
		task_list->head = new_task;
		task_list->priority_heads[priority] = new_task;
		task_list->priority_tails[priority] = new_task;
	}
	// If it is the new highest priority
	else if (task_list->head->priority > priority) {
		task_list->head = new_task;
		task_list->priority_heads[priority] = new_task;
		task_list->priority_tails[priority] = new_task;

		// Link tail
		for (i = priority + 1; i < TASK_PRIORITY_MAX; i++) {
			if (task_list->priority_heads[i] != NULL) {
				new_task->next = task_list->priority_heads[i];
				break;
			}
		}
	}
	// If it is not new highest, but new priority
	else if (task_list->priority_tails[priority] == NULL) {
		task_list->priority_heads[priority] = new_task;
		task_list->priority_tails[priority] = new_task;
		// Link higher priority task next to new_task
		for (i = priority - 1; i >= 0; i--) {
			if (task_list->priority_tails[i] != NULL) {
				new_task->next = task_list->priority_tails[i]->next;
				task_list->priority_tails[i]->next = new_task;
				break;
			}
		}
		// assert(i >= 0, "TaskList: Failed to find higher priority task");
	}
	// If not new priority
	else {
		new_task->next = task_list->priority_tails[priority]->next;
		task_list->priority_tails[priority]->next = new_task;
		task_list->priority_tails[priority] = new_task;
	}

	DEBUG(DB_TASK, "| TASK:\tInserted\tTid: %d SP: 0x%x\n", new_task->tid, new_task->current_sp);
	return 1;
}

void removeCurrentTask(TaskList *task_list, FreeList *free_list) {
	assert(task_list->curtask == task_list->head, "Trying to remove a curtask that is not a head. ");
	Task *task = task_list->curtask;
	int priority = task->priority;

	// Adjust top_priority head and tails
	if (task_list->priority_heads[priority] == task_list->priority_tails[priority]) {
		task_list->priority_heads[priority] = NULL;
		task_list->priority_tails[priority] = NULL;
	} else {
		task_list->priority_heads[priority] = task_list->priority_heads[priority]->next;
	}

	// Move head to next and clear curtask
	task_list->head = task_list->head->next;
	task_list->curtask = NULL;

	// Add the task to free list
	if (free_list->head == NULL) {
		free_list->head = task;
		free_list->tail = task;
	} else {
		free_list->tail->next = task;
		free_list->tail = task;
	}

	// Change the task state to Zombie and next to NULL (tail of free_list)
	assert(task->state == Active, "Current task state is not Active");
	task->state = Zombie;
	task->next = NULL;
}

Task *createTask(FreeList *free_list, int priority, void (*code) ()) {
	assert(priority >= 0 && priority <= TASK_PRIORITY_MAX, "Invalid priority value!");
	Task *ret = NULL;
	ret = free_list->head;
	if(ret == NULL) return ret;

	// assert(ret->state == Empty, "Invalid task space to use!");
	ret->tid = ret->tid + TASK_MAX;
	ret->state = Ready;
	ret->priority = priority;

	free_list->head = free_list->head->next;

	// Link to next ready task and parent tid should be set when pushing to task list
	ret->next = NULL;
	ret->parent_tid = -1;

	ret->current_sp = initTrap(ret->init_sp, (TEXT_REG_BASE + Exit), (TEXT_REG_BASE + code));
	DEBUG(DB_TASK, "| Task:\tCreated\t\tTid: 0x%x SP: 0x%x, LR: 0x%x\n", ret->tid, ret->current_sp, (TEXT_REG_BASE + code));
	return ret;
}

void moveCurrentTaskToEnd(TaskList *task_list) {
	int priority = task_list->curtask->priority;

	// Change the task state to Ready
	assert(task_list->curtask->state == Active, "Current task state is not Active");
	task_list->curtask->state = Ready;

	if (task_list->priority_heads[priority] != task_list->priority_tails[priority]) {
		if (task_list->head->priority < priority) {
			task_list->head->next = task_list->curtask->next;
		} else {
			task_list->head = task_list->curtask->next;
		}
		task_list->priority_heads[priority] = task_list->curtask->next;
		task_list->curtask->next = task_list->priority_tails[priority]->next;
		task_list->priority_tails[priority]->next = task_list->curtask;
		task_list->priority_tails[priority] = task_list->curtask;
	}
}

void refreshCurtask(TaskList *task_list) {
	task_list->curtask = task_list->head;
	// Change the task state to Active
	task_list->curtask->state = Active;
}

int scheduleNextTask(TaskList *tlist) {
	if(tlist->curtask != NULL) {
		moveCurrentTaskToEnd(tlist);
	}
	refreshCurtask(tlist);
	if (tlist->curtask == NULL) {
		return 0;
	}
	DEBUG(DB_TASK, "| TASK:\tScheduled\tTid: %d Priority: %d\n", tlist->curtask->tid, tlist->curtask->priority);
	return 1;
}

void enqueueBlockedList(BlockedList *blocked_lists, int blocked_list_index, Task* task) {
	// If no blocked list to enqueue, return
	if (blocked_lists == NULL) {
		return;
	}

	if (blocked_lists[blocked_list_index].head == NULL) {
		blocked_lists[blocked_list_index].head = task;
		blocked_lists[blocked_list_index].tail = task;
	} else {
		blocked_lists[blocked_list_index].tail->next = task;
		blocked_lists[blocked_list_index].tail = task;
	}
}

int dequeueBlockedList(BlockedList *blocked_lists, int blocked_list_index) {
	int ret = -1;
	Task *read_task = NULL;
	if (blocked_lists[blocked_list_index].head != NULL) {
		read_task = blocked_lists[blocked_list_index].head;
		ret = read_task->tid;
		blocked_lists[blocked_list_index].head = blocked_lists[blocked_list_index].head->next;
		if (blocked_lists[blocked_list_index].head == NULL) {
			blocked_lists[blocked_list_index].tail = NULL;
		}
	}
	return ret;
}

void blockCurrentTask(TaskList *task_list, TaskState blocked_state,
                      BlockedList *blocked_lists, int blocked_list_index) {
	int top_priority = task_list->head->priority;
	Task *task = task_list->curtask;
	assert(task_list->head == task_list->curtask, "Blocking syscall invalid");

	// Adjust top_priority head and tails
	if (task_list->priority_heads[top_priority] == task_list->priority_tails[top_priority]) {
		task_list->priority_heads[top_priority] = NULL;
		task_list->priority_tails[top_priority] = NULL;
	} else {
		task_list->priority_heads[top_priority] = task_list->priority_heads[top_priority]->next;
	}
	task_list->head = task_list->head->next;

	assert(task_list->curtask->state == Active, "Current task state is not Active");
	task_list->curtask->state = blocked_state;
	task_list->curtask->next = NULL;
	task_list->curtask = NULL;

	enqueueBlockedList(blocked_lists, blocked_list_index, task);
}

void blockedListsInitial(BlockedList *blocked_lists, int list_num) {
	int i = -1;
	for (i = 0; i < list_num; i++) {
		blocked_lists[i].head = NULL;
		blocked_lists[i].tail = NULL;
	}
}

void msgArrayInitial(MsgBuffer *msg_array) {
	int i = -1;
	for (i = 0; i < TASK_MAX; i++) {
		msg_array[i].msg = NULL;
		msg_array[i].reply = NULL;
		msg_array[i].event = NULL;
	}
}
