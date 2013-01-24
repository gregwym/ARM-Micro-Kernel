#include <bwio.h>
#include <syscall.h>
#include <syscall_handler.h>
#include <stdlib.h>

void sysCreate(TaskList *tlist, FreeList *flist, int priority, void * context()) {
	DEBUG(DB_SYSCALL, "I WANNA CREATE!!!!!\n");
	Task *task = createTask(flist, priority, context);
	insertTask(tlist, task);
}

void sysExit(TaskList *tlist, FreeList *flist) {
	DEBUG(DB_SYSCALL, "I WANNA EXIT!!!!!\n");
	removeCurrentTask(tlist, flist);
}

void syscallHandler(void **parameters, KernelGlobal *global, void *user_sp, void *user_resume_point) {
	int callno = *((int*)(parameters[0]));
	DEBUG(DB_SYSCALL, "Call number: %d\n", callno);
	DEBUG(DB_SYSCALL, "global: 0x%x\n", global);
	DEBUG(DB_SYSCALL, "user_sp: 0x%x\n", user_sp);
	DEBUG(DB_SYSCALL, "user_resume_point: 0x%x\n", user_resume_point);

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

	return callno;
}

