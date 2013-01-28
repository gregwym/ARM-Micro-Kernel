#ifndef _KERN_TRAPFRAME_H_
#define _KERN_TRAPFRAME_H_

void kernelEntry();
void kernelExit(void *resume_point);
void switchCpuMode(int flag);
void *initTrap(void *sp, void *exit_syscall);
void activateStack(void *sp);

void syscallHandler();

#endif // _KERN_TRAPFRAME_H_
