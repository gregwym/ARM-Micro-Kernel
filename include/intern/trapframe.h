#ifndef _KERN_TRAPFRAME_H_
#define _KERN_TRAPFRAME_H_

typedef struct user_trap {
	int spsr;
	int resume_point;
	int r0;
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int sb;
	int sl;
	int fp;
	int ip;
	int sp;
	int lr;
} UserTrapframe;

void irqEntry();
void swiEntry();
void kernelExit(void *user_sp);
void *initTrap(void *sp, void *exit_syscall, void *user_resume_point);

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp);
int syscallHandler(void **parameters, KernelGlobal *global);
void irqHandler(KernelGlobal *global);

#endif // _KERN_TRAPFRAME_H_
