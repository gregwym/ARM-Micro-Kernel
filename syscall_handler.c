#include <bwio.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <stdlib.h>

void sysExit() {
	DEBUG(DB_SYSCALL, "I WANNA EXIT!!!!!\n");
}

void syscallHandler() {
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
		default:
			break;
	}

	// asm("sub	sp, fp, #16");
	// asm("ldmfd	sp, {sl, fp, sp}");
	// kernelExit(DATA_REGION_BASE + Exit);
}

