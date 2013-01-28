#include <kern/callno.h>
#include <kern/errno.h>
#include <kern/types.h>
#include <task.h>
#include <klib.h>

int sysCreate(TaskList *task_list, FreeList *free_list, int priority, void (*code) (), int *rtn) {
	Task *task = createTask(free_list, priority, code);
	if(task == NULL) return -1;

	task->parent_tid = task_list->curtask->tid;
	insertTask(task_list, task);
	*rtn = task->tid;
	return 0;
}

int sysExit(TaskList *task_list, FreeList *free_list) {
	removeCurrentTask(task_list, free_list);
	return 0;
}

int sysMyTid(TaskList *task_list, int *rtn) {
	*rtn = task_list->curtask->tid;
	return 0;
}

int sysMyParentTid(TaskList *tlist, int *rtn) {
	*rtn = tlist->curtask->parent_tid;
	return 0;
}

int sysSend(KernelGlobal *global, int tid, char *msg, int msglen, char *reply, int replylen, int *rtn) {

	TaskList 	*task_list = global->task_list;
	BlockedList *blocked_list = global->blocked_list;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int cur_tid = task_list->curtask->tid;

	// not a possible task id.
	if (tid < 0) {
		*rtn = -1;
		return 0;
	}

	// not an existing task
	if (task_array[tid % TASK_MAX].tid != tid) {
		DEBUG(DB_MSG_PASSING, "SYSSEND: task id not existing\n");
		*rtn = -2;
		return 0;
	}

	assert((msg_array[cur_tid]).msg == NULL, "msg_array is not NULL");
	// link sender's msg to msg_array
	(msg_array[cur_tid]).msg = msg;
	(msg_array[cur_tid]).msglen = msglen;
	(msg_array[cur_tid]).reply = reply;
	(msg_array[cur_tid]).replylen = replylen;

	// block current task and add it to blocklist
	addToBlockedList(blocked_list, task_list, tid);

	// unblock the receiver task
	if (task_array[tid % TASK_MAX].state == SendBlocked) {
		task_array[tid % TASK_MAX].state = Ready;
		insertTask(task_list, &task_array[tid % TASK_MAX]);
	}

	*rtn = 0;
	return 0;
}

int sysReceive(KernelGlobal *global, int *tid, char *msg, int msglen, int *rtn) {

	TaskList 	*task_list = global->task_list;
	BlockedList *blocked_list = global->blocked_list;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int sender_tid = -1;
	Task *sender_task = NULL;

	// pull a sender's tid
	sender_tid = getFromBlockedList(blocked_list, task_list->curtask);

	if (sender_tid == -1) {
		// no one send current task msg
		blockCurrentTask(task_list, SendBlocked);
		*rtn = 0;
		// bwprintf(COM2, "Receiver block itself\n");
		return 0;
	}

	// sender has sent msg to current task
	sender_task = &(task_array[sender_tid % TASK_MAX]);
	assert(sender_task->state == ReceiveBlocked, "Receiver got an invalid sender");
	assert(msg_array[sender_tid % TASK_MAX].msg != NULL, "Receiver got an NULL msg");

	// set tid to sender's tid
	*tid = sender_tid;

	// set return value to sender's msg length
	*rtn = msg_array[sender_tid % TASK_MAX].msglen;

	// copy msg to receive msg buffer
	msg = memcpy(msg, msg_array[sender_tid % TASK_MAX].msg, msglen);

	// unblock the sender and make it replyblocked
	sender_task->state = ReplyBlocked;

	return 0;
}


int sysReply(KernelGlobal *global, int tid, char *reply, int replylen, int *rtn) {

	// assert(task_array[tid % TASK_MAX].tid == tid, "Reply to an unexisting task");

	TaskList 	*task_list = global->task_list;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	// not a possible task id
	if (tid < 0) {
		*rtn = -1;
		return 0;
	}

	// not an existing task
	if (task_array[tid % TASK_MAX].tid != tid) {
		*rtn = -2;
		return 0;
	}

	// sender is no reply blocked
	if (task_array[tid % TASK_MAX].state != ReplyBlocked) {
		*rtn = -3;
		return 0;
	}

	Task *sender_task = &task_array[tid % TASK_MAX];

	// copy reply msg to reply buffer
	msg_array[tid % TASK_MAX].reply = memcpy(msg_array[tid % TASK_MAX].reply, reply, msg_array[tid % TASK_MAX].replylen);
	msg_array[tid % TASK_MAX].msg = NULL;

	// set sender's return value to be the reply msg's length
	*((int *)sender_task->current_sp) = replylen;

	// put sender back to ready queue
	sender_task->state = Ready;
	insertTask(task_list, sender_task);

	*rtn = 0;
	return 0;
}

void syscallHandler(void **parameters, KernelGlobal *global, void *user_sp, void *user_resume_point) {
	int callno = *((int*)(parameters[0]));
	int err = 0;
	int rtn = 0;

	DEBUG(DB_SYSCALL, "| SYSCALL:\tCall number: %d\n", callno);
	// DEBUG(DB_SYSCALL, "| SYSCALL:\tglobal: 0x%x\n", global);
	// DEBUG(DB_SYSCALL, "| SYSCALL:\tuser_sp: 0x%x\n", user_sp);
	// DEBUG(DB_SYSCALL, "| SYSCALL:\tuser_resume_point: 0x%x\n", user_resume_point);

	TaskList *task_list = global->task_list;
	FreeList *free_list = global->free_list;

	global->task_list->curtask->current_sp = user_sp;
	global->task_list->curtask->resume_point = user_resume_point;

	switch(callno) {
		case SYS_exit:
			err = sysExit(task_list, free_list);
			break;
		case SYS_create:
			err = sysCreate(task_list, free_list, *((int*)(parameters[1])), *((void **)(parameters[2])), &rtn);
			break;
		case SYS_myTid:
			err = sysMyTid(task_list, &rtn);
			break;
		case SYS_myParentTid:
			err = sysMyParentTid(task_list, &rtn);
			break;
		case SYS_send:
			err = sysSend(global, *((int*)(parameters[1])), (char*)parameters[2], *((int*)(parameters[3])),
						(char*)parameters[4], *((int*)(parameters[5])), &rtn);
			break;
		case SYS_receive:
			err = sysReceive(global, (int*)parameters[1], (char*)parameters[2], *((int*)(parameters[3])), &rtn);
			break;
		case SYS_reply:
			err = sysReply(global, *((int*)(parameters[1])), (char*)parameters[2], *((int*)(parameters[3])), &rtn);
			break;
		case SYS_pass:
		default:
			break;
	}

	*((int *)user_sp) = (err == 0 ? rtn : err);

	return;
}

