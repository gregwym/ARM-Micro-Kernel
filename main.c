#include <klib.h>
#include <unistd.h>
#include <intern/trapframe.h>
#include <kern/types.h>
#include <kern/md_const.h>

void user_program_iner() {
	bwprintf(COM2, "myTid: %d myParentTid: %d\n", MyTid(), MyParentTid());
	Pass();
	bwprintf(COM2, "myTid: %d myParentTid: %d\n", MyTid(), MyParentTid());
	Exit();
}

void sender() {
	int ret = -1;
	char msg[9];
	msg[0] = 'h';
	msg[1] = 'e';
	msg[2] = 'l';
	msg[3] = 'l';
	msg[4] = 'o';
	msg[5] = '\0';
	char reply[9];
	bwprintf(COM2, "Sender call send\n");
	ret = Send(2, msg, 9, reply, 5);
	bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
	Exit();
}

void sr() {
	int ret = -1;
	int ret2;
	int tid;
	char rmsg[5];
	char replymsg[5];
	replymsg[0] = 'm';
	replymsg[1] = 'i';
	replymsg[2] = 'd';
	replymsg[3] = '\0';

	char msg[9];
	msg[0] = 'm';
	msg[1] = 'i';
	msg[2] = 'd';
	msg[3] = 's';
	msg[4] = 'e';
	msg[5] = 'n';
	msg[6] = 'd';
	msg[7] = '\0';
	char reply[9];
	while (1) {
		bwprintf(COM2, "sr call receive\n");
		ret = Receive(&tid, rmsg, 5);
		if (ret > 0) {
			bwprintf(COM2, "sr receive: %s with origin %d char\n", rmsg, ret);
			bwprintf(COM2, "sr reply to %d\n", tid);
			ret2 = Reply(tid, replymsg, 5);
			bwprintf(COM2, "sr ret value: %d\n", ret2);
			ret = Send(1, msg, 9, reply, 2);
			bwprintf(COM2, "sr receive reply: %s with origin %d char\n", reply, ret);

		}
	}
}

void receiver() {
	int ret = -1;
	int ret2;
	int tid;
	char msg[5];
	char replymsg[5];
	replymsg[0] = 'c';
	replymsg[1] = 'a';
	replymsg[2] = 'o';
	replymsg[3] = '\0';
	while (1) {
		bwprintf(COM2, "Receiver call receive\n");
		ret = Receive(&tid, msg, 5);
		if (ret > 0) {
			bwprintf(COM2, "Receive msg: %s with origin %d char\n", msg, ret);
			bwprintf(COM2, "Receiver reply to %d\n", tid);
			ret2 = Reply(tid, replymsg, 5);
			bwprintf(COM2, "Reply ret value: %d\n", ret2);
		}
	}
}

void user_program() {
	int tid = -1;

	tid = Create(7, DATA_REGION_BASE + receiver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, DATA_REGION_BASE + sr);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(3, DATA_REGION_BASE + sender);
	bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(7, DATA_REGION_BASE + sender);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(7, DATA_REGION_BASE + sender);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(7, DATA_REGION_BASE + sender);
	// bwprintf(COM2, "Created: %d\n", tid);

	// tid = Create(3, DATA_REGION_BASE + user_program_iner);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(3, DATA_REGION_BASE + user_program_iner);
	// bwprintf(COM2, "Created: %d\n", tid);

	bwprintf(COM2, "First: exiting\n");
	// while (1) {
		// Pass();
	// }
	Exit();
}

int scheduleNextTask(TaskList *tlist) {
	if(tlist->curtask != NULL) {
		moveCurrentTaskToEnd(tlist);
	}
	refreshCurtask(tlist);
	if (tlist->curtask == NULL) {
		return 0;
	}
	activateStack(tlist->curtask->current_sp);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tUser task activated, sp: 0x%x\n", tlist->curtask->current_sp);
	return 1;
}

int main() {
	bwsetfifo(COM2, OFF);

	/* Initialize TaskList */
	TaskList tlist;
	FreeList flist;
	Task task_array[TASK_MAX];
	char stacks[TASK_MAX * TASK_STACK_SIZE];
	Task *priority_head[TASK_PRIORITY_MAX];
	Task *priority_tail[TASK_PRIORITY_MAX];
	BlockedList blocked_list[TASK_MAX];
	MsgBuffer msg_array[TASK_MAX];

	tarrayInitial(task_array, stacks);
	flistInitial(&flist, task_array);
	tlistInitial(&tlist, priority_head, priority_tail);
	blockedListInitial (blocked_list);
	msgArrayInitial (msg_array);

	/* Setup global kernel entry */
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) (DATA_REGION_BASE + kernelEntry);

	/* Setup kernel global variable structure */
	KernelGlobal global;
	global.task_list = &tlist;
	global.free_list = &flist;
	global.blocked_list = blocked_list;
	global.msg_array = msg_array;
	global.task_array = task_array;

	/* Set spsr to usermode */
	asm("mrs	r12, spsr");
	asm("bic 	r12, r12, #0x1f");
	asm("orr 	r12, r12, #0x10");
	asm("msr 	SPSR_c, r12");

	/* Create first task */
	Task *first_task = createTask(&flist, 5, DATA_REGION_BASE + user_program);
	insertTask(&tlist, first_task);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tFirst task created, init_sp: 0x%x\n", first_task->init_sp);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tGlobal addr: 0x%x\n", &global);

	/* Main syscall handling loop */
	while(1){
		// If no more task to run, break
		if(!scheduleNextTask(&tlist)) break;
		Task *tmp = tlist.curtask;
		while (tmp != NULL) {
			// bwprintf(COM2, "%d -> ", tmp->tid);
			tmp = tmp->next;
		}
		// Exit kernel to let user program to execute
		kernelExit(tlist.curtask->resume_point);
		asm("mov r1, %0"
		    :
		    :"r"(&global)
		    :"r0", "r2", "r3"
		    );
		asm("bl	syscallHandler(PLT)");

		DEBUG(DB_SYSCALL, "| SYSCALL:\tSyscall Handler returned normally, exiting kernel\n");
	}

	return 0;
}
