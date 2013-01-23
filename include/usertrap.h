void kernelEntry();
void kernelExit(void *exit_syscall);
void switchCpuMode(int flag);
void initTrap(void *sp, void * context());
void activateStack(void *sp);
