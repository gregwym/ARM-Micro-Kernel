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
	int callno = SYS_create;
	void *parameters[4] = {(&callno), (&priority), (&code), NULL};
	asm("mov r0, %0"
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
	int rtn = -1;
	int callno = SYS_myTid;
	void *parameters[2] = {(&callno), NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");

	// Return the return value from kernel
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);
	return rtn;
}

int MyParentTid() {

}

void Pass() {
	int callno = SYS_pass;
	void *parameters[2] = {(&callno), NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 2");
}

void Exit() {
	int callno = SYS_exit;
	void *parameters[2] = {(&callno), NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
}
