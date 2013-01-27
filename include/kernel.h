#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <task.h>

typedef struct kernel_global {
	TaskList 	*task_list;
	FreeList 	*free_list;
	BlockedList	*blocked_list;
	MsgBuffer	*msg_array;
	Task	 	*task_array;
} KernelGlobal;

#define TASK_MAX 30
#define TASK_STACK_SIZE 512
#define TASK_PRIORITY_MAX 10

#define SWI_ENTRY_POINT 0x28
#define DATA_REGION_BASE 0x218000

#define CPU_MODE_USER 0x10
#define CPU_MODE_SVC 0x13
#define CPU_MODE_SYS 0x1f

#endif // __KERNEL_H__
