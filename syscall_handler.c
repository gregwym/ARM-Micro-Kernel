#include <bwio.h>
#include <syscall_handler.h>
#include <stdlib.h>

void sysExit() {

}

void syscallHandler() {
	int callno = -1;
	int *lr = 0;
	asm("mov %0, r4"
		:"=r"(lr)
		:
	);

	asm("ldr r3, [r4, #-4]");
	asm("mov %0, r3"
		:"=r"(callno)
		:
	);
	callno = callno & 0xffffff;

	bwprintf(COM2, "lr: 0x%x Call number: %d\n", lr, callno);

	switch(callno) {
		case SYS_exit:
			sysExit();
			break;
		default:
			break;
	}

}

