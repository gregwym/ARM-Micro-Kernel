#include <bwio.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <stdlib.h>

void sysCreate(TaskList *tlist, FreeList *flist, int priority, void (*code) ()) {
	Task *task = createTask(flist, priority, code);
	insertTask(tlist, task);
}

void sysExit(TaskList *tlist, FreeList *flist) {
	removeCurrentTask(tlist, flist);
}

void syscallHandler(void **parameters, KernelGlobal *global, void *user_sp, void *user_resume_point) {
	int callno = *((int*)(parameters[0]));
	DEBUG(DB_SYSCALL, "| SYSCALL:\tCall number: %d\n", callno);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tglobal: 0x%x\n", global);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tuser_sp: 0x%x\n", user_sp);
	DEBUG(DB_SYSCALL, "| SYSCALL:\tuser_resume_point: 0x%x\n", user_resume_point);

	TaskList *tlist = global->tlist;
	FreeList *flist = global->flist;

	global->tlist->curtask->current_sp = user_sp;
	global->tlist->curtask->resume_point = user_resume_point;

	switch(callno) {
		case SYS_exit:
			sysExit(tlist, flist);
			break;
		case SYS_create:
			sysCreate(tlist, flist, *((int*)(parameters[1])), *((void **)(parameters[2])));
			break;
		default:
			break;
	}

	return;
}

