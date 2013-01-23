#include <bwio.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <stdlib.h>

void sysCreate() {
	DEBUG(DB_SYSCALL, "I WANNA CREATE!!!!!\n");
}

void sysExit() {
	DEBUG(DB_SYSCALL, "I WANNA EXIT!!!!!\n");
}

int syscallHandler() {
	int callno = -1;
	asm("ldr %0, [r0, #-4]"
		:"=r"(callno)
		:
	);

	callno = callno & 0xffffff;

	bwprintf(COM2, "Call number: %d\n", callno);

	switch(callno) {
		case SYS_exit:
			sysExit();
			break;
		case SYS_create:
			sysCreate();
			break;
		default:
			break;
	}

	return callno;
}

