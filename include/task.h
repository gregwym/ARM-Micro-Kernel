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
	TaskState	state;						// task state
	int			priority;					// priority
	void		*init_sp;					// stack initial position
	void		*current_sp;				// stack current position
	struct		task_descriptor *next;		// next task
	int			parent_tid;					// parent
	// SPSR
	void		*resume_point;				// pc addr to resume task
	// Return value
} Task;

typedef struct task_list {
	Task 		*curtask;					// Task pointer to the current task/swi caller
	Task 		*head;						// Task pointer to the first highest priority task
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

int insertTask(TaskList *tlist, Task *new_task);

Task *createTask(FreeList *flist, int priority, void * context());

Task *removeCurrentTask(TaskList *tlist, FreeList *flist);

void moveCurrentTaskToEnd(TaskList *tlist);

void refreshCurtask(TaskList *tlist);


#endif //__TASK_H__
