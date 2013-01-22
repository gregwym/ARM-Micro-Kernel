#include <bwio.h>
#include <stdlib.h>
#include <syscall.h>
#include <usertrap.h>

void printCPSR(){
	int cpsr;
	asm("mrs r3, CPSR");
	asm("mov %0, R3"
		:"=r"(cpsr)
		:
	);
	cpsr = cpsr & 0x1f;
	DEBUG(DB_SYSCALL, "USER: CPSR 0x%x\n", cpsr);
}

void user_program2() {
	printCPSR();
	bwprintf(COM2, "In user program2\n");
}

void user_program() {
	printCPSR();
	bwprintf(COM2, "In user program\n");

	Create(0, DATA_REGION_BASE + user_program2);

	bwprintf(COM2, "Back to user program\n");
	printCPSR();
}

int main() {
	bwsetfifo(COM2, OFF);
	printCPSR();

	// int fp;
	// asm("mov %0, FP"
	// 	:"=r"(fp)
	// 	:
	// );
	// DEBUG(DB_SYSCALL, "KERNEL: FP 0x%x\n", fp);

	// Initialize Kernel Global vairable's storage
	// KernelGlobal kernel_global;

	// Initialize TaskList
	// TaskList task_list;
	// Task tasks[TASK_MAX];
	// Task *priority_head[TASK_PRIORITY_MAX];
	// Task *priority_tail[TASK_PRIORITY_MAX];
	char stack[TASK_MAX * TASK_STACK_SIZE];
	// tlistInitial(&task_list, tasks, priority_head, priority_tail, stack);
	// kernel_global.task_list = &task_list;

	// bwprintf(COM2, "Task list at 0x%x\n", kernel_global.task_list);

	// Save kernel_global address on the top of stack
	// asm("str %0, [sp, #-4]"
	// 	:
	// 	:"r"(&kernel_global));
	// asm("sub sp, sp, #4");

	// Setup global kernel entry
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) DATA_REGION_BASE + kernelEntry;

	// Allocate user_sp for the first user_program
	char *user_sp = &(stack[TASK_STACK_SIZE - 4]);

	// switch to system mode
	switchCpuMode(CPU_MODE_SYS);

	// write sp
	asm("mov sp, %0"
		:
		:"r"(user_sp)
	);

	bwprintf(COM2, "In system mode, initialize user_sp to 0x%x\n", user_sp);

	// back to svc mode
	switchCpuMode(CPU_MODE_SVC);

	bwprintf(COM2, "In svc mode\n");

	// Switch to USER mode and start user_program()
	switchCpuMode(CPU_MODE_USER);
	user_program();

	// Cannot switch back since USER mode does not has the privilege
	// switchCpuMode(CPU_MODE_SVC);

	printCPSR();
	bwprintf(COM2, "Came back to main\n");

	// asm("mov %0, FP"
	// 	:"=r"(fp)
	// 	:
	// );
	// DEBUG(DB_SYSCALL, "KERNEL: FP 0x%x\n", fp);

	return 0;
}
