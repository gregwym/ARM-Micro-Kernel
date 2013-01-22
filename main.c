#include <bwio.h>
#include <stdlib.h>
#include <syscall.h>

void printCPSR(){
	int cpsr;
	asm("mrs r3, CPSR");
	asm("mov %0, R3"
		:"=r"(cpsr)
		:
	);
	cpsr = cpsr & 0x1f;
	DEBUG(DB_SYSCALL, "USER: CPSR 0x%x SP 0x%x\n", cpsr, &cpsr);
}

inline void switchCpuMode(int flag) {
	asm("MRS R12, CPSR");
	asm("BIC R12, R12, #0x1F");
	switch(flag){
		case CPU_MODE_SVC:
			asm("ORR R12, R12, #0x13"); break;
		case CPU_MODE_SYS:
			asm("ORR R12, R12, #0x1f"); break;
		case CPU_MODE_USER:
		default:
			asm("ORR R12, R12, #0x10");
	}
	asm("MSR CPSR_c, R12");
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
	// asm("MOV PC, LR");
}

int main() {
	bwsetfifo(COM2, OFF);

	int sp = 0;

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

	// Create(0, user_program);


	int *usersp = &(stack[TASK_STACK_SIZE - 4]);


	//switch to system mode
	switchCpuMode(CPU_MODE_SYS);

	bwprintf(COM2, "In system mode\n");

	//write sp
	asm("mov sp, %0"
		:
		:"r"(usersp)
	);

	bwprintf(COM2, "Set usersp to 0x%x\n", usersp);

	//bk to svc mode
	switchCpuMode(CPU_MODE_SVC);

	bwprintf(COM2, "In svc mode, sp 0x%x\n", &sp);

	//adjust SPSR
	asm("MRS R12, CPSR");
	asm("BIC R12, R12, #0x1F");
	asm("ORR R12, R12, #0x10");
	asm("MSR SPSR_c, R12");

	bwprintf(COM2, "SPSR Adjusted\n");

	//switch to user mode
	asm("MOVS PC, %0"
		:
		:"r"(DATA_REGION_BASE + user_program));
	// user_program();

	bwprintf(COM2, "Came back to main\n");

	return 0;
}
