#include <klib.h>
#include <unistd.h>
#include <intern/trapframe.h>
#include <kern/types.h>
#include <kern/md_const.h>
#include <nameserver.h>

// Prototype for the user program main function
void umain();

#ifndef umain
void umain() {
	bwprintf(COM2, "Hello World!\n");
}
#endif

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
	Task *first_task = createTask(&flist, 5, DATA_REGION_BASE + umain);
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
