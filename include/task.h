#ifndef __TASK_H__
#define __TASK_H__

typedef enum TaskState { 
	Active,
	Ready,
	Zombie
} TaskState;

typedef struct task_descripter {
	int 		tf[16];						//trapframe
	int 		tid;						//id
	TaskState 	state;						//task state
	int 		priority;					//priority
	void 		*position;					//poistion in the stack
	struct 		task_descripter *next;		//next task
	struct 		task_descripter *parent;	//parent
} Task;

typedef struct task_list {
	int 		list_size;
	int 		list_counter;
	int 		max_plvl;
	Task 		*tasklist;
	Task 		*head;
	Task 		**priority_head;
	Task 		**priority_tail;
} TaskList;


int tlistInitial (TaskList *tlist, Task *tasks, int tlist_size, Task **head, Task **tail, int max_p, char* stack);

int tlistPush (TaskList *tlist, void *context, int priority);

Task* tlistPop(TaskList *tlist);

#endif //__TASK_H__