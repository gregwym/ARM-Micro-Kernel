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

int main() {
	/* Initialize hardware */
	// Turn off interrupt
	asm("msr 	CPSR_c, #0xd3");

	// Turn off FIFO
	bwsetfifo(COM2, OFF);

	// Setup timer
	// setTimerLoadValue(TIMER3_BASE, 0x00000fff);
	// setTimerControl(TIMER3_BASE, TRUE, TRUE, FALSE);

	// Setup VIC
	// setVicEnable(VIC1_BASE);
	// setVicEnable(VIC2_BASE);
	// unsigned int *vic2_in_en_addr = (unsigned int *) (VIC2_BASE + VIC_IN_EN_OFFSET);
	// *vic2_in_en_addr = VIC_TIMER3_MASK;

	/* Initialize TaskList */
	TaskList tlist;
	FreeList flist;
	Task task_array[TASK_MAX];
	char stacks[TASK_MAX * TASK_STACK_SIZE];
	Task *priority_head[TASK_PRIORITY_MAX];
	Task *priority_tail[TASK_PRIORITY_MAX];
	BlockedList receive_blocked_lists[TASK_MAX];
	BlockedList event_blocked_lists[EVENT_MAX];
	MsgBuffer msg_array[TASK_MAX];

	tarrayInitial(task_array, stacks);
	flistInitial(&flist, task_array);
	tlistInitial(&tlist, priority_head, priority_tail);
	blockedListsInitial(receive_blocked_lists, TASK_MAX);
	blockedListsInitial(event_blocked_lists, EVENT_MAX);
	msgArrayInitial(msg_array);

	/* Setup global kernel entry */
	int *swi_entry = (int *) SWI_ENTRY_POINT;
	*swi_entry = (int) (TEXT_REG_BASE + swiEntry);
	int *irq_entry = (int *) IRQ_ENTRY_POINT;
	*irq_entry = (int) (TEXT_REG_BASE + irqEntry);

	/* Setup kernel global variable structure */
	KernelGlobal global;
	global.task_list = &tlist;
	global.free_list = &flist;
	global.receive_blocked_lists = receive_blocked_lists;
	global.event_blocked_lists = event_blocked_lists;
	global.msg_array = msg_array;
	global.task_array = task_array;

	/* Set spsr to usermode with IRQ and FIQ on */
	// asm("msr 	SPSR_c, #0x10");

	/* Create first task */
	Task *first_task = createTask(&flist, 0, umain);
	insertTask(&tlist, first_task);

	/* Main syscall handling loop */
	while(1){
		// If no more task to run, break
		if(!scheduleNextTask(&tlist)) break;
		UserTrapframe* user_sp = (UserTrapframe *)tlist.curtask->current_sp;
		DEBUG(DB_SYSCALL, "| SYSCALL:\tEXITING SP: 0x%x SPSR: 0x%x ResumePoint: 0x%x\n", user_sp, user_sp->spsr, user_sp->resume_point);
		// Exit kernel to let user program to execute
		kernelExit(tlist.curtask->current_sp);

		asm("mov r1, %0"
		    :
		    :"r"(&global)
		    :"r0", "r2", "r3"
		    );
		asm("bl	syscallHandler(PLT)");
	}

	return 0;
}
