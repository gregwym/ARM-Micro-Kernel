#include <types.h>
#include <task.h>
#include <kernel.h>
#include <bwio.h>
#include <usertrap.h>
#include <syscall.h>
#include <stdlib.h>

void task_listInitial(TaskList *task_list, Task **heads, Task **tails) {
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
	}
}

void free_listInitial(FreeList *free_list, Task *task_array) {
	int i;
	for (i = 0; i < TASK_MAX - 1; i++) {
		task_array[i].next = &(task_array[i+1]);
		task_array[i].state = Empty;
	}
	task_array[TASK_MAX - 1].next = NULL;
	task_array[TASK_MAX - 1].state = Empty;

	free_list->head = &task_array[0];
	free_list->tail = &task_array[TASK_MAX - 1];
}

int insertTask(TaskList *task_list, Task *new_task) {
	int i;
	int priority = new_task->priority;

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

	DEBUG(DB_TASK, "Task(tid: %d) is created, stack at %x\n", new_task->tid, new_task->init_sp);
	return 1;
}

Task* removeCurrentTask(TaskList *task_list, FreeList *free_list) {
	// assert(task_list->head != NULL, "TaskList: list is empty");
	int top_priority = task_list->head->priority;
	Task *ret = NULL;

	// Adjust top_priority head and tails
	if (task_list->priority_heads[top_priority] == task_list->priority_tails[top_priority]) {
		task_list->priority_heads[top_priority] = NULL;
		task_list->priority_tails[top_priority] = NULL;
	} else {
		task_list->priority_heads[top_priority] = task_list->priority_heads[top_priority]->next;
	}

	ret = task_list->head;
	
	if (task_list->head == task_list->curtask) {
		task_list->head = task_list->head->next;
	}

	if (free_list->head == NULL) {
		free_list->head = ret;
		free_list->tail = ret;
		ret->next = NULL;
		ret->state = Empty;
	} else {
		free_list->tail->next = ret;
		free_list->tail = ret;
		free_list->tail->next = NULL;
		free_list->tail->state = Empty;
	}
	task_list->curtask = NULL;
	return ret;
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

	ret->resume_point = code;
	ret->current_sp = initTrap(ret->init_sp, DATA_REGION_BASE + Exit);

	return ret;
}

void moveCurrentTaskToEnd(TaskList *task_list) {
	int priority = task_list->curtask->priority;

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
}

void addToBlockedList (BlockList *blocked_list, Task *cur_task, int receiver_tid) {
	int index = receiver_tid % TASK_MAX;
	cur_task->next = NULL;
	if (blocked_list[index].head == NULL) {
		blocked_list[index].head = cur_task;
		blocked_list[index].tail = cur_task;
	} else {
		blocked_list[index].tail->next = cur_task;
		blocked_list[index].tail = cur_task;
	}
}

int getFromBlockedList (BlockList *blocked_list, Task *cur_task) {
	int index = cur_task->tid % TASK_MAX;
	int ret = -1;
	Task *read_task = NULL;
	if (blocked_list[index].head != NULL) {
		read_task = blocked_list[index].head;
		read_task->state = ReplyBlocked;
		ret = read_task->tid;
		blocked_list[index].head = blocked_list[index].head->next;
		if (blocked_list[index].head == NULL) {
			blocked_list[index].tail = NULL;
		}
	}
	return ret;
}
		

void blockCurrentTask(TaskList *task_list, TaskState state) {
	int top_priority = task_list->head->priority;
	assert(task_list->head == task_list->curtask, "Blocking syscall invalid");

	// Adjust top_priority head and tails
	if (task_list->priority_heads[top_priority] == task_list->priority_tails[top_priority]) {
		task_list->priority_heads[top_priority] = NULL;
		task_list->priority_tails[top_priority] = NULL;
	} else {
		task_list->priority_heads[top_priority] = task_list->priority_heads[top_priority]->next;
	}
	
	task_list->head = task_list->head->next;
	
	task_list->curtask->state = state;
	task_list->curtask = NULL;
}



void blockedListInitial (BlockList *block_list) {
	int i = -1;
	for (i = 0; i < TASK_MAX; i++) {
		block_list[i].head = NULL;
		block_list[i].tail = NULL;
	}
}

void msgArrayInitial (MsgBuffer *msg_array) {
	int i = -1;
	for (i = 0; i < TASK_MAX; i++) {
		msg_array[i].msg = NULL;
		msg_array[i].reply = NULL;
	}
}

	
	
	

	
	
	
	
	
	
	
	
	