#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <intern/trapframe.h>
#include <task.h>

typedef struct kernel_global {
	ReadyQueue 	*ready_queue;
	FreeList 	*free_list;
	BlockedList	*receive_blocked_lists;
	BlockedList	*event_blocked_lists;
	MsgBuffer	*msg_array;
	Task	 	*task_array;
} KernelGlobal;

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp);
int syscallHandler(void **parameters, KernelGlobal *global);
void irqHandler(KernelGlobal *global);

#endif // _INTERRUPT_H_
