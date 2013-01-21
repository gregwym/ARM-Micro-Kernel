#ifndef __TASK_H__
#define __TASK_H__

typedef enum TaskState { 
	Active,
	Ready,
	Zombie, 
	Empty
} TaskState;

typedef struct task_descripter {
	int 		tid;						//id
	TaskState 	state;						//task state
	int 		priority;					//priority
	void 		*init_sp;					//poistion in the stack
	struct 		task_descripter *next;		//next task
	int 		parent_tid;					//parent
} Task;

typedef struct task_list {
	int 		list_counter;				// New insert task index (no free)
	Task 		*task_array;				// 
	Task 		*head;						// Task pointer to the head of list
	Task 		**priority_heads;			// Task pointers to the head of each priority
	Task 		**priority_tails;			// Task pointers to the tail of each priority
} TaskList;


int tlistInitial (TaskList *tlist, Task *task_array, Task **heads, Task **tails, char* stacks);

int tlistPush (TaskList *tlist, void *context, int priority);

Task* tlistPop(TaskList *tlist);

#endif //__TASK_H__
