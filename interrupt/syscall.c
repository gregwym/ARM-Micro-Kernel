#include <kern/callno.h>
#include <kern/errno.h>
#include <klib.h>
#include <intern/trapframe.h>

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
	BlockedList *receive_blocked_lists = global->receive_blocked_lists;
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
	enqueueBlockedList(receive_blocked_lists, tid % TASK_MAX, task_list, ReceiveBlocked);

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
	BlockedList *receive_blocked_lists = global->receive_blocked_lists;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int sender_tid = -1;
	Task *sender_task = NULL;

	// pull a sender's tid
	sender_tid = dequeueBlockedList(receive_blocked_lists, task_list->curtask->tid % TASK_MAX);

	if (sender_tid == -1) {
		// no one send current task msg
		blockCurrentTask(task_list, SendBlocked);
		return -1;
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
	TaskList 	*task_list = global->task_list;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	// Not a possible task id
	if (tid < 0) {
		return -1;
	}

	// Not an existing task
	if (task_array[tid % TASK_MAX].tid != tid) {
		return -2;
	}

	// Sender is no reply blocked
	if (task_array[tid % TASK_MAX].state != ReplyBlocked) {
		return -3;
	}

	Task *sender_task = &task_array[tid % TASK_MAX];

	// copy reply msg to reply buffer
	msg_array[tid % TASK_MAX].reply = memcpy(msg_array[tid % TASK_MAX].reply, reply, msg_array[tid % TASK_MAX].replylen);
	msg_array[tid % TASK_MAX].msg = NULL;

	// Set sender's return value to be the reply msg's length
	UserTrapframe *sender_trapframe = (UserTrapframe *)sender_task->current_sp;
	sender_trapframe->r0 = replylen;

	// Put sender back to ready queue
	sender_task->state = Ready;
	insertTask(task_list, sender_task);

	// Reply succeed
	*rtn = 0;
	return 0;
}

int sysAwaitEvent(KernelGlobal *global, int eventid, char *event, int eventlen, int *rtn) {
	TaskList 	*task_list = global->task_list;
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	MsgBuffer 	*msg_array = global->msg_array;

	int cur_tid = task_list->curtask->tid;
	assert((msg_array[cur_tid]).event == NULL, "MsgBuffer.event is not NULL");
	// Link waiter's event and eventlen to msg_array
	(msg_array[cur_tid]).event = event;
	(msg_array[cur_tid]).eventlen = eventlen;

	// Block current task and add it to blocklist
	enqueueBlockedList(event_blocked_lists, eventid, task_list, EventBlocked);

	*rtn = 0;
	return 0;
}

int syscallHandler(void **parameters, KernelGlobal *global) {
	int callno = *((int*)(parameters[0]));
	int err = 0;
	int rtn = 0;

	DEBUG(DB_SYSCALL, "| SYSCALL:\tCallno: %d\n", callno);

	TaskList *task_list = global->task_list;
	FreeList *free_list = global->free_list;

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
		case SYS_awaitEvent:
			err = sysAwaitEvent(global, *((int*)(parameters[1])), *((char**)parameters[2]), *((int*)(parameters[3])), &rtn);
			break;
		case SYS_pass:
		default:
			break;
	}

	return (err == 0 ? rtn : err);
}
