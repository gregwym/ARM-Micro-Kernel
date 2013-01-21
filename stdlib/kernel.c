#include <bwio.h>
#include <task.h>
#include <kernel.h>
#include <types.h>

KernelGlobal *getKernelGlobal() {
	KernelGlobal *p = NULL;
	asm("ldr %0, _KERNEL_GLOBAL"
		:"=r"(p)
		:);
	return p;
}
