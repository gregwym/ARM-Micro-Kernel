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

int syscallHandler(int callno, void **parameters, void *user_sp, void *user_resume_point) {

	DEBUG(DB_SYSCALL, "Call number: %d\n", callno);

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

