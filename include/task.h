#ifndef __TASK_H__
#define __TASK_H__

#define TASK_MAX 30
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY_MAX 10

typedef enum TaskState {
	Active,
	Ready,
	Zombie,
	SendBlocked,
	ReceiveBlocked,
	ReplyBlocked,
	EventBlocked
} TaskState;

typedef struct task_descriptor {
	int			tid;						// id
	TaskState	state;						// task state
	int			priority;					// priority
	void		*init_sp;					// stack initial position
	void		*current_sp;				// stack current position
	struct		task_descriptor *next;		// next task
	int			parent_tid;					// parent

	/* The following value are saved on user stack, within the trapframe */
	// SPSR
	// pc addr to resume task
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

typedef struct block_list {
	Task		*head;
	Task		*tail;
} BlockedList;

typedef struct msg_buffer {
	char *msg;
	int msglen;
	char *reply;
	int replylen;
	char *event;
	int eventlen;
} MsgBuffer;



void tlistInitial(TaskList *tlist, Task **heads, Task **tails);

void tarrayInitial(Task *task_array, char *stacks);

void flistInitial(FreeList *flist, Task *task_array);

int insertTask(TaskList *tlist, Task *new_task);

Task *createTask(FreeList *flist, int priority, void (*code) ());

void removeCurrentTask(TaskList *tlist, FreeList *flist);

// move current task to the end of its priority queue
void moveCurrentTaskToEnd(TaskList *tlist);

// set current task to head
void refreshCurtask(TaskList *tlist);

// Schedule next task to run, return 0 if no more task
int scheduleNextTask(TaskList *tlist);

// Add current task to specified block list
void enqueueBlockedList(BlockedList *blocked_lists, int blocked_list_index, Task *task);

// receiver call this to get a sender's tid that has sent to the receiver
int dequeueBlockedList(BlockedList *blocked_lists, int blocked_list_index);

// block current task with "state" and add to the blocked_list
// (if blocked_list is not NULL)
void blockCurrentTask(TaskList *task_list, TaskState blocked_state,
                      BlockedList *blocked_lists, int blocked_list_index);

// block list initialization
void blockedListsInitial(BlockedList *block_lists, int list_num);

// msg array initialization
void msgArrayInitial(MsgBuffer *msg_array);

void createIdleTask();

#endif //__TASK_H__
