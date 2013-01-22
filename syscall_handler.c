#include <bwio.h>
#include <syscall_no.h>
#include <stdlib.h>


void syscall_handler() {
	

	// asm("add sp, sp, #4");
	// asm("ldr r3, [sp, #-4]");

	int callno = -1;
	int *lr = 0;
	// KernelGlobal *kernel_global = NULL;
	asm("mov %0, lr"
		:"=r"(lr)
		:
	);

	asm("ldr r3, [lr, #-4]");
	asm("mov %0, r3"
		:"=r"(callno)
		:
		:"r3"
	);
	callno = callno & 0xffffff;

	// asm("mov %0, r3"
	// 	:"=r"(kernel_global)
	// 	:);
	// bwprintf(COM2, "Kernel global variables: 0x%x, task_list 0x%x\n", kernel_global, kernel_global->task_list);
	bwprintf(COM2, "lr: 0x%x Call number: %d\n", lr, callno);

	int cpsr = -1;
	asm("mrs r3, CPSR");
	asm("mov %0, R3"
		:"=r"(cpsr)
		:
	);
	cpsr = cpsr & 0x1f;
	bwprintf(COM2, "CPSR 0x%x\n", cpsr);

	int spsr = -1;
	asm("mrs r3, SPSR");
	asm("mov %0, R3"
		:"=r"(spsr)
		:
	);
	spsr = spsr & 0x1f;
	bwprintf(COM2, "SPSR 0x%x\n", spsr);

	asm("MOVS pc, %0"
		:
		:"r"(lr));
}

