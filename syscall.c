#include <syscall_no.h>
#include <stdlib.h>
#include <syscall_handler.h>
#include <bwio.h>
#include <usertrap.h>

int Create(int priority, void (*code) ()) {
	int *lr = 0;
	asm("mov %0, lr"
		:"=r"(lr)
		:
	);

	int *pc;
	asm("mov %0, pc"
		:"=r"(pc)
		:
	);

	int *fp;
	asm("mov %0, fp"
		:"=r"(fp)
		:
	);
	bwprintf(COM2, "CREATE: PC 0x%x, lr 0x%x, fp 0x%x\n", pc, lr, fp);

	asm("swi 1");
	bwprintf(COM2, "Got back\n");
	bwprintf(COM2, "Got back next line\n");

	// asm("MOV pc, %0"
	// 	:
	// 	:"r"(lr));

	asm("mov %0, lr"
		:"=r"(lr)
		:
	);

	asm("mov %0, pc"
		:"=r"(pc)
		:
	);

	asm("mov %0, fp"
		:"=r"(fp)
		:
	);
	bwprintf(COM2, "CREATE: PC 0x%x, lr 0x%x, fp 0x%x\n", pc, lr, fp);

	return 0;
}
