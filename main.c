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

void client() {
	int ret = -1;
	char msg[9];
	msg[0] = 'R';
	msg[1] = 'f';
	msg[2] = 'i';
	msg[3] = 'r';
	msg[4] = 's';
	msg[5] = 't';
	msg[6] = NULL;
	bwprintf(COM2, "Sender call send\n");
	ret = RegisterAs(msg);
	switch (ret) {
		case 0:
			bwprintf(COM2, "Register successfully with name %s: \n", &msg[1]);
			break;
		default:
			bwprintf(COM2, "Register exception\n");
			break;
	}
	msg[0] = 'W';
	ret = WhoIs(msg);
	if (ret == 0) {
		bwprintf(COM2, "%s does exist in name server\n", &msg[1]);
	} else {
		bwprintf(COM2, "%s is the task with %d\n", &msg[1], ret);
	}

	// bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
	Exit();
}

void client2() {
	int ret = -1;
	char msg[9];
	msg[0] = 's';
	msg[1] = 'f';
	msg[2] = 'i';
	msg[3] = 'r';
	msg[4] = 's';
	msg[5] = 't';
	msg[6] = NULL;
	bwprintf(COM2, "Sender call send\n");
	ret = RegisterAs(msg);
	switch (ret) {
		case 0:
			bwprintf(COM2, "Register successfully with name %s: \n", &msg[1]);
			break;
		case -2:
			bwprintf(COM2, "Name %s has been registered\n" , &msg[1]);
			break;
		case -1:
			bwprintf(COM2, "Name server tid invalid\n");
		default:
			break;
	}

	// bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
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
			ret = Reply(tid, replymsg, 5);
			bwprintf(COM2, "Reply ret value: %d\n", ret);
		}
	}
}

typedef struct ns {
	char name[10];
	int tid;
} Name_Server;

int search_table(Name_Server *table, char *msg, int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (strcmp(msg, table[i].name) == 0) {
			bwprintf(COM2, "hit\n");
			return i;
		}
	}
	return -1;
}

void name_server() {
	int ret = -1;
	int tid;
	char msg[10];
	char replymsg[5];
	char replymsg_y[5];
	replymsg_y[4] = NULL;

	char replymsg_n[5];
	*(int *)replymsg_y = 0;
	replymsg_n[4] = NULL;

	// U: invalid input format
	char replymsg_u[5];
	*(int *)replymsg_u[0] = -1;
	replymsg_u[4] = NULL;


	Name_Server table[10];
	int i;
	for (i = 0; i < 10; i++) {
		table[i].name[0] = NULL;
	}

	int ns_counter = 0;

	while (1) {
		bwprintf(COM2, "Nameserver call receive\n");
		ret = Receive(&tid, msg, 10);
		if (ret > 0) {
			if (msg[0] == 'R') {
				if (search_table(table, &msg[1], ns_counter) != -1) {
					table[i].tid = tid;
					strncpy(table[i].name, &msg[1], 10);
					*(int *)replymsg_y = 1;
					ret = Reply(tid, replymsg_y, 5);
				} else {
					table[ns_counter].tid = tid;
					strncpy(table[ns_counter].name, &msg[1], 10);
					*((int *)replymsg_y) = 1;
					ret = Reply(tid, replymsg_y, 5);
					bwprintf(COM2, "NS: reply num: %d\n", *((int *)replymsg_y));
					bwprintf(COM2, "NS: reply num[0]: %d\n", replymsg_y[0]);
					bwprintf(COM2, "NS: reply num[1]: %d\n", replymsg_y[1]);
					bwprintf(COM2, "NS: reply num[2]: %d\n", replymsg_y[2]);
					bwprintf(COM2, "NS: reply num[3]: %d\n", replymsg_y[3]);


					bwprintf(COM2, "NS: reply msg size: %d\n", strlen(replymsg_y));
					ns_counter++;
				}
			}
			else if (msg[0] == 'W') {
				int i = search_table(table, &msg[1], ns_counter);
				if (i == -1) {
					ret = Reply(tid, replymsg_n, 5);
				} else {
					*(int *)replymsg_y = table[i].tid;
					ret = Reply(tid, replymsg_y, 5);
				}
			}
			else {
				ret = Reply(tid, replymsg_u, 5);
			}
		}
	}
}




void user_program() {
	int tid = -1;

	tid = Create(1, DATA_REGION_BASE + name_server);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, DATA_REGION_BASE + client);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, DATA_REGION_BASE + client);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, DATA_REGION_BASE + client2);
	bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(3, DATA_REGION_BASE + sender);
	// bwprintf(COM2, "Created: %d\n", tid);
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
