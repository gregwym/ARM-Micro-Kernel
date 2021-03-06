#include <unistd.h>
#include <klib.h>
#include <task.h>
#include <kern/callno.h>

int Create(int priority, void (*code) ()) {
	return CreateWithArgs(priority, code, 0, 0, 0, 0);
}

int CreateWithArgs(int priority, void (*code) (), int a1, int a2, int a3, int a4) {
	int rtn = -1;
	// If priority is invalid, return -1
	if (priority < 0 || priority >= TASK_PRIORITY_MAX ) {
		return rtn;
	}

	// Setup parameters and callno
	int callno = SYS_create;
	int args_array[] = {a1, a2, a3, a4, 0};
	int *args = args_array;
	void *parameters[5] = {(&callno), (&priority), (&code), (&args), NULL};
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
	int rtn = -1;
	int callno = SYS_myParentTid;
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

int Send( int tid, char *msg, int msglen, char *reply, int replylen ) {
	int callno = SYS_send;
	int rtn = -1;
	void *parameters[7] = {(&callno), &tid, &msg, &msglen, &reply, &replylen, NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);
	return rtn;
}

int Receive( int *tid, char *msg, int msglen ) {
	int rtn = -1;
	int callno = SYS_receive;
	void *parameters[5] = {(&callno), &tid, &msg, &msglen, NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);

	assert(rtn >= 0, "Received with ERROR");

	return rtn;
}

int Reply( int tid, char *reply, int replylen ) {
	int rtn = -1;
	int callno = SYS_reply;
	void *parameters[5] = {(&callno), &tid, &reply, &replylen, NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);
	return rtn;
}

int AwaitEvent( int eventid, char *event, int eventlen ) {
	int rtn = -1;
	int callno = SYS_awaitEvent;
	void *parameters[5] = {(&callno), &eventid, &event, &eventlen,  NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
	asm("mov %0, r0"
	    :"=r"(rtn)
	    :);
	return rtn;
}

void Halt() {
	int callno = SYS_halt;
	void *parameters[2] = {(&callno), NULL};
	asm("mov r0, %0"
	    :
	    :"r"(parameters));
	asm("swi 0");
}
