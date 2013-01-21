#include <bwio.h>
#include <task.h>
#include <kernel.h>
#include <types.h>

int user_program() {
	bwprintf(COM2, "In user program\n");
	return 0;
}

int main() {

	// Initialize Kernel Global vairable's storage
	asm("_KERNEL_GLOBAL: .word 0");
	KernelGlobal kernel_global;
	asm("str %0, _KERNEL_GLOBAL"
		:
		:"r"(&kernel_global));

	// Initialize TaskList
	TaskList task_list;
	Task tasks[TASK_MAX];
	Task *priority_head[TASK_PRIORITY_MAX];
	Task *priority_tail[TASK_PRIORITY_MAX];
	char stack[TASK_MAX * TASK_STACK_SIZE];
	tlistInitial(&task_list, tasks, priority_head, priority_tail, stack);
	kernel_global.task_list = &task_list;


	
	return 0;
}
