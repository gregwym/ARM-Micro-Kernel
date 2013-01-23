#include <bwio.h>
#include <stdlib.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <usertrap.h>
#include <global_ref.h>

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

	// Create(0, DATA_REGION_BASE + user_program2);

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

	/* Initialize TaskList */
	TaskList tlist;
	FreeList flist;
	Task task_array[TASK_MAX];
	char stacks[TASK_MAX * TASK_STACK_SIZE];
	Task *priority_head[TASK_PRIORITY_MAX];
	Task *priority_tail[TASK_PRIORITY_MAX];

	tarrayInitial(task_array, stacks);
	flistInitial(&flist, task_array);
	tlistInitial(&tlist, priority_head, priority_tail);

	/* Setup global kernel entry */
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) (DATA_REGION_BASE + kernelEntry);

	/* Set spsr to usermode */
	asm("mrs	r12, spsr");
	asm("bic 	r12, r12, #0x1f");
	asm("orr 	r12, r12, #0x10");
	asm("msr 	SPSR_c, r12");

	/* Create first task */
	Task *first_task = createTask(&flist, 0, DATA_REGION_BASE + user_program);
	DEBUG(DB_SYSCALL, "First task created, init_sp: 0x%x\n", first_task->init_sp);

	// tlistPush(&tlist, first_task);

	/* Main syscall handling loop */
	while(1){
		// Exit kernel to let user program to execute
		kernelExit(DATA_REGION_BASE + Exit);
		// Return to this point after swi
		syscallHandler();
		DEBUG(DB_SYSCALL, "Syscall Handler returned normally, exiting kernel\n");
	}

	printCPSR();
	bwprintf(COM2, "Came back to main\n");

	// asm("mov %0, FP"
	// 	:"=r"(fp)
	// 	:
	// );
	// DEBUG(DB_SYSCALL, "KERNEL: FP 0x%x\n", fp);

	return 0;
}
