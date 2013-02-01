#ifndef _KERN_TRAPFRAME_H_
#define _KERN_TRAPFRAME_H_

void swiEntry();
void kernelExit(void *resume_point);
void switchCpuMode(int flag);
void *initTrap(void *sp, void *exit_syscall);
void activateStack(void *sp);

void syscallHandler(void **parameters, KernelGlobal *global, void *user_sp, void *user_resume_point);

#endif // _KERN_TRAPFRAME_H_
