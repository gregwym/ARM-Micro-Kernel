#include <stdlib.h>
#include <syscall_handler.h>
#include <bwio.h>
#include <usertrap.h>

int Create(int priority, void (*code) ()) {
	int rtn = -1;
	// If priority is invalid, return -1
	if (priority < 0 || priority >= TASK_PRIORITY_MAX ) {
		return rtn;
	}

	// Setup parameters and callno
	void *parameters[3] = {(&priority), (&code), NULL};
	asm("mov r0, #1");
	asm("mov r1, %0"
	    :
	    :"r"(parameters));

	// Enter kernel
	asm("swi 1");

	// Return the return value from kernel
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);
	return rtn;
}

int MyTid() {

}

int MyParentTid() {

}

void Pass() {

}

void Exit() {
	asm("mov r0, #0");
	asm("swi 0");
}
