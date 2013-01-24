#include <bwio.h>
#include <stdlib.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <usertrap.h>

void user_program_low() {
	bwprintf(COM2, "In user program low before\n");
	Pass();
	bwprintf(COM2, "In user program low after\n");
}

void user_program_high() {
	bwprintf(COM2, "In user program high before\n");
	Pass();
	bwprintf(COM2, "In user program high after\n");
}

void user_program() {
	bwprintf(COM2, "In main user program\n");

	Create(6, DATA_REGION_BASE + user_program_low);
	bwprintf(COM2, "Created low 1\n");
	Create(7, DATA_REGION_BASE + user_program_low);
	bwprintf(COM2, "Created low 2\n");

	Create(4, DATA_REGION_BASE + user_program_high);
	bwprintf(COM2, "Created high 1\n");
	Create(3, DATA_REGION_BASE + user_program_high);
	bwprintf(COM2, "Created high 2\n");

	bwprintf(COM2, "First: exiting\n");
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

	tarrayInitial(task_array, stacks);
	flistInitial(&flist, task_array);
	tlistInitial(&tlist, priority_head, priority_tail);

	/* Setup global kernel entry */
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) (DATA_REGION_BASE + kernelEntry);

	/* Setup kernel global variable structure */
	KernelGlobal global;
	global.tlist = &tlist;
	global.flist = &flist;

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
