#ifndef __KERNEL_H__
#define __KERNEL_H__

typedef struct kernel_global {
	TaskList *task_list;

} KernelGlobal;
KernelGlobal *getKernelGlobal();

#define TASK_MAX 30
#define TASK_STACK_SIZE 1000
#define TASK_PRIORITY_MAX 10

#endif // __KERNEL_H__
