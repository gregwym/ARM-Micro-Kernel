#include <interrupt.h>
#include <klib.h>
#include <ts7200.h>
#include <kern/callno.h>
#include <kern/errno.h>
#include <kern/unistd.h>

int sysCreate(KernelGlobal *global, int priority, void (*code) (), int *args, int *rtn) {
	ReadyQueue *ready_queue = global->ready_queue;
	FreeList *free_list = global->free_list;

	Task *task = createTask(free_list, priority, code);
	if(task == NULL) return -1;

	global->task_stat.total_created++;
	if(task->tid > global->task_stat.max_tid) {
		global->task_stat.max_tid = task->tid;
	}

	task->parent_tid = ready_queue->curtask->tid;
	insertTask(ready_queue, task);

	UserTrapframe *trapframe = (UserTrapframe *)task->current_sp;
	trapframe->r0 = args[0];
	trapframe->r1 = args[1];
	trapframe->r2 = args[2];
	trapframe->r3 = args[3];

	*rtn = task->tid;
	return 0;
}

int sysExit(ReadyQueue *ready_queue, FreeList *free_list) {
	removeCurrentTask(ready_queue, free_list);
	return 0;
}

int sysMyTid(ReadyQueue *ready_queue, int *rtn) {
	*rtn = ready_queue->curtask->tid;
	return 0;
}

int sysMyParentTid(ReadyQueue *ready_queue, int *rtn) {
	*rtn = ready_queue->curtask->parent_tid;
	return 0;
}

int sysSend(KernelGlobal *global, int tid, char *msg, int msglen, char *reply, int replylen, int *rtn) {

	ReadyQueue 	*ready_queue = global->ready_queue;
	BlockedList *receive_blocked_lists = global->receive_blocked_lists;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int cur_tid = ready_queue->curtask->tid;
	int task_index = cur_tid % TASK_MAX;
	int dst_index = tid % TASK_MAX;

	// not a possible task id.
	if (tid < 0) {
		*rtn = -1;
		return 0;
	}

	// not an existing task
	if (task_array[dst_index].tid != tid) {
		*rtn = -2;
		return 0;
	}

	assert((msg_array[task_index]).msg == NULL, "msg_array is not NULL");

	// if receiver task is already SendBlocked, unblock the receiver task and pass msg into receiver's buffer
	if (task_array[dst_index].state == SendBlocked) {
		DEBUG(DB_SYSCALL, "Sender writing to receiver tid buffer addr 0x%x\n", msg_array[dst_index].sender_tid_ref);
		memcpy(msg_array[dst_index].receive, msg, msg_array[dst_index].receivelen);
		*(msg_array[dst_index].sender_tid_ref) = cur_tid;

		// edit receiver task's return value
		((UserTrapframe *)(task_array[dst_index].current_sp))->r0 = msglen;

		blockCurrentTask(ready_queue, ReplyBlocked, NULL, 0);
		insertTask(ready_queue, &(task_array[dst_index]));
	} else {
		// link sender's msg to msg_array
		(msg_array[task_index]).msg = msg;
		(msg_array[task_index]).msglen = msglen;

		// block current task and add it to blocklist
		blockCurrentTask(ready_queue, ReceiveBlocked, receive_blocked_lists, tid % TASK_MAX);
	}

	(msg_array[task_index]).reply = reply;
	(msg_array[task_index]).replylen = replylen;

	*rtn = 0;
	return 0;
}

int sysReceive(KernelGlobal *global, int *tid, char *msg, int msglen, int *rtn) {

	ReadyQueue 	*ready_queue = global->ready_queue;
	BlockedList *receive_blocked_lists = global->receive_blocked_lists;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int task_index = ready_queue->curtask->tid % TASK_MAX;
	int sender_tid = -1;
	Task *sender_task = NULL;

	// pull a sender's tid
	sender_tid = dequeueBlockedList(receive_blocked_lists, ready_queue->curtask->tid % TASK_MAX);

	if (sender_tid == -1) {
		// no one send current task msg
		msg_array[task_index].receive = msg;
		msg_array[task_index].receivelen = msglen;
		msg_array[task_index].sender_tid_ref = tid;
		blockCurrentTask(ready_queue, SendBlocked, NULL, 0);
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
	ReadyQueue 	*ready_queue = global->ready_queue;
	MsgBuffer 	*msg_array = global->msg_array;
	Task	 	*task_array = global->task_array;

	int dst_index = tid % TASK_MAX;

	// Not a possible task id
	if (tid < 0) {
		return -1;
	}

	// Not an existing task
	if (task_array[dst_index].tid != tid) {
		return -2;
	}

	// Sender is no reply blocked
	if (task_array[dst_index].state != ReplyBlocked) {
		return -3;
	}

	Task *sender_task = &task_array[dst_index];

	// copy reply msg to reply buffer
	memcpy(msg_array[dst_index].reply, reply, msg_array[dst_index].replylen);
	msg_array[dst_index].msg = NULL;

	// Set sender's return value to be the reply msg's length
	UserTrapframe *sender_trapframe = (UserTrapframe *)sender_task->current_sp;
	sender_trapframe->r0 = replylen;

	// Put sender back to ready queue
	insertTask(ready_queue, sender_task);

	// Reply succeed
	*rtn = 0;
	return 0;
}

int sysAwaitEvent(KernelGlobal *global, int eventid, char *event, int eventlen, int *rtn) {
	ReadyQueue 	*ready_queue = global->ready_queue;
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	MsgBuffer 	*msg_array = global->msg_array;

	if (eventid < 0 || eventid >= EVENT_MAX) {
		return -1;
	}

	int task_index = ready_queue->curtask->tid % TASK_MAX;
	assert((msg_array[task_index]).event == NULL, "MsgBuffer.event is not NULL");

	// Link waiter's event and eventlen to msg_array
	(msg_array[task_index]).event = event;
	(msg_array[task_index]).eventlen = eventlen;

	// Block current task and add it to blocklist
	blockCurrentTask(ready_queue, EventBlocked, event_blocked_lists, eventid);

	// Turn on UART IRQ if is an COM event
	switch(eventid) {
		case EVENT_COM2_TX:
			setUARTControlBit(UART2_BASE, TIEN_MASK, TRUE);
			break;
		case EVENT_COM2_RX:
			setUARTControlBit(UART2_BASE, RIEN_MASK, TRUE);
			break;
		case EVENT_COM1_TX:
			if(!global->uart1_waiting_cts)
				setUARTControlBit(UART1_BASE, TIEN_MASK, TRUE);
			break;
		case EVENT_COM1_RX:
			setUARTControlBit(UART1_BASE, RIEN_MASK, TRUE);
			break;
		default:
			break;
	}

	*rtn = 0;
	return 0;
}

int sysHalt(KernelGlobal *global) {
	ReadyQueue 	*ready_queue = global->ready_queue;
	ready_queue->curtask = NULL;
	ready_queue->head = NULL;
	return 0;
}

int syscallHandler(void **parameters, KernelGlobal *global) {
	int callno = *((int*)(parameters[0]));
	int err = 0;
	int rtn = 0;

	DEBUG(DB_SYSCALL, "| SYSCALL:\tCallno: %d\n", callno);

	ReadyQueue *ready_queue = global->ready_queue;
	FreeList *free_list = global->free_list;

	switch(callno) {
		case SYS_exit:
			err = sysExit(ready_queue, free_list);
			break;
		case SYS_create:
			err = sysCreate(global, *((int*)(parameters[1])), *((void **)(parameters[2])), *((int **)(parameters[3])), &rtn);
			break;
		case SYS_myTid:
			err = sysMyTid(ready_queue, &rtn);
			break;
		case SYS_myParentTid:
			err = sysMyParentTid(ready_queue, &rtn);
			break;
		case SYS_send:
			err = sysSend(global, *((int*)(parameters[1])), *((char**)parameters[2]), *((int*)(parameters[3])),
						*((char**)parameters[4]), *((int*)(parameters[5])), &rtn);
			break;
		case SYS_receive:
			err = sysReceive(global, *((int**)parameters[1]), *((char**)parameters[2]), *((int*)(parameters[3])), &rtn);
			break;
		case SYS_reply:
			err = sysReply(global, *((int*)(parameters[1])), *((char**)parameters[2]), *((int*)(parameters[3])), &rtn);
			break;
		case SYS_awaitEvent:
			err = sysAwaitEvent(global, *((int*)(parameters[1])), *((char**)parameters[2]), *((int*)(parameters[3])), &rtn);
			break;
		case SYS_halt:
			err = sysHalt(global);
			break;
		case SYS_pass:
		default:
			break;
	}

	return (err == 0 ? rtn : err);
}

