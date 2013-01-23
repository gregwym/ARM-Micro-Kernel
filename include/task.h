#ifndef __TASK_H__
#define __TASK_H__

typedef enum TaskState {
	Active,
	Ready,
	Zombie,
	Empty
} TaskState;

typedef struct task_descriptor {
	int			tid;						// id
	int			generation;					// generation
	TaskState	state;						// task state
	int			priority;					// priority
	void		*init_sp;					// stack initial position
	void		*current_sp;				// stack current position
	struct		task_descriptor *next;		// next task
	int			parent_tid;					// parent
} Task;

typedef struct task_list {
	Task 		*head;						// Task pointer to the head of list
	Task 		**priority_heads;			// Task pointers to the head of each priority
	Task 		**priority_tails;			// Task pointers to the tail of each priority
} TaskList;

typedef struct free_list {
	Task		*head;
	Task		*tail;
} FreeList;


void tlistInitial (TaskList *tlist, Task **heads, Task **tails);

void tarrayInitial(Task *task_array, char *stacks);

void flistInitial(FreeList *flist, Task *task_array);

int pushTask(TaskList *tlist, Task *new_task);

Task *createTask(FreeList *flist, int priority, void * context());

Task *popTask(TaskList *tlist, FreeList *flist);


#endif //__TASK_H__
