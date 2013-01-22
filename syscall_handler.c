#include <bwio.h>
#include <syscall_no.h>
#include <stdlib.h>


void syscallHandler() {
	int callno = -1;
	int *lr = 0;
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

