#include <klib.h>
#include <unistd.h>
#include <intern/trapframe.h>
#include <kern/md_const.h>
#include <kern/ts7200.h>

#define TRUE	0xffffffff
#define FALSE	0x00000000

unsigned int setTimerLoadValue(int timer_base, unsigned int value) {
	unsigned int* timer_load_addr = (unsigned int*) (timer_base + LDR_OFFSET);
	*timer_load_addr = value;
	return *timer_load_addr;
}

unsigned int setTimerControl(int timer_base, unsigned int enable, unsigned int mode, unsigned int clksel) {
	unsigned int* timer_control_addr = (unsigned int*) (timer_base + CRTL_OFFSET);
	unsigned int control_value = (ENABLE_MASK & enable) | (MODE_MASK & mode) | (CLKSEL_MASK & clksel) ;
	// DEBUG(DB_TIMER, "Timer3 control changing from 0x%x to 0x%x.\n", *timer_control_addr, control_value);

	*timer_control_addr = control_value;
	return *timer_control_addr;
}

unsigned int getTimerValue(int timer_base) {
	unsigned int* timer_value_addr = (unsigned int*) (timer_base + VAL_OFFSET);
	unsigned int value = *timer_value_addr;
	return value;
}

unsigned int setVicEnable(int vic_base) {
	unsigned int* vic_enable_addr = (unsigned int*) (vic_base + VIC_IN_EN_OFFSET);
	*vic_enable_addr = vic_base == VIC1_BASE ? 0x7F7FFFF : vic_base == VIC2_BASE ? 0xF97865FB : 0x0;
	return *vic_enable_addr;
}

// Prototype for the user program main function
void umain();

#ifndef umain
void umain() {
	unsigned int i = 1;
	bwprintf(COM2, "Hello World!\n");
	while(i++) {
		unsigned int *vic2_irq_addr = (unsigned int*) VIC2_BASE + VIC_IRQ_ST_OFFSET;
		if(i % 100000 == 0) bwprintf(COM2, "0x%x irq 0x%x\n", getTimerValue(TIMER3_BASE), *vic2_irq_addr);
	}
}
#endif

void irqEntry() {
	unsigned int cpsr;
	asm("mrs 	%0, CPSR"
	    :"=r"(cpsr)
	    :
	    );
	unsigned int i = 1;
	unsigned int *vic2_irq_addr = (unsigned int*) VIC2_BASE + VIC_IRQ_ST_OFFSET;
	unsigned int *timer3_in_en_addr = (unsigned int *) (VIC2_BASE + VIC_IN_EN_OFFSET);
	unsigned int *timer3_clr_addr = (unsigned int*) TIMER3_BASE + CLR_OFFSET;
	unsigned int *timer3_in_enc_addr = (unsigned int *) (VIC2_BASE + VIC_IN_ENC_OFFSET);

	bwprintf(COM2, "Catch IRQ with CPSR 0x%x and IRQ flags 0x%x, VIC2EN 0x%x\n", cpsr, *vic2_irq_addr, *timer3_in_en_addr);
	*timer3_clr_addr = 0xff;
	*timer3_in_enc_addr = 0x00080000;
	*timer3_in_en_addr = 0x00000000;
	*timer3_in_en_addr = 0x00080000;
	bwprintf(COM2, "IRQ flags 0x%x, VIC2EN 0x%x\n", *vic2_irq_addr, *timer3_in_en_addr);
	// while(i++ < 100000);

	asm("movs pc, lr");
}


int main() {
	/* Initialize hardware */
	bwsetfifo(COM2, OFF);
	setTimerLoadValue(TIMER3_BASE, 0x00000fff);
	setTimerControl(TIMER3_BASE, TRUE, TRUE, FALSE);
	// setVicEnable(VIC1_BASE);
	// setVicEnable(VIC2_BASE);
	unsigned int *timer3_in_en_addr = (unsigned int *) (VIC2_BASE + VIC_IN_EN_OFFSET);
	*timer3_in_en_addr = 0x00080000;

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
	blockedListInitial(blocked_list);
	msgArrayInitial(msg_array);

	/* Setup global kernel entry */
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) (DATA_REGION_BASE + kernelEntry);
	int *irq_entry = (int *) IRQ_ENTRY_POINT;
	*irq_entry = (int) (DATA_REGION_BASE + irqEntry);

	/* Setup kernel global variable structure */
	KernelGlobal global;
	global.task_list = &tlist;
	global.free_list = &flist;
	global.blocked_list = blocked_list;
	global.msg_array = msg_array;
	global.task_array = task_array;

	/* Set spsr to usermode */
	asm("msr 	SPSR_c, #0x10");

	/* Create first task */
	Task *first_task = createTask(&flist, 0, umain);
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
