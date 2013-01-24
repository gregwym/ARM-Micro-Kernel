void kernelEntry();
void kernelExit(void *resume_point);
void switchCpuMode(int flag);
void *initTrap(void *sp, void *exit_syscall);
void activateStack(void *sp);
