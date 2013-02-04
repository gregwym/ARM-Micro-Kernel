#include <klib.h>
#include <unistd.h>
#include <interrupt.h>
#include <kern/md_const.h>
#include <kern/ts7200.h>

#define TRUE	0xffffffff
#define FALSE	0x00000000
#define TIMER_TICK_SIZE	20

unsigned int setTimerLoadValue(int timer_base, unsigned int value) {
	unsigned int* timer_load_addr = (unsigned int*) (timer_base + LDR_OFFSET);
	*timer_load_addr = value;
	return *timer_load_addr;
}

unsigned int setTimerControl(int timer_base, unsigned int enable, unsigned int mode, unsigned int clksel) {
	unsigned int* timer_control_addr = (unsigned int*) (timer_base + CRTL_OFFSET);
	unsigned int control_value = (ENABLE_MASK & enable) | (MODE_MASK & mode) | (CLKSEL_MASK & clksel) ;

	*timer_control_addr = control_value;
	return *timer_control_addr;
}

unsigned int getTimerValue(int timer_base) {
	unsigned int* timer_value_addr = (unsigned int*) (timer_base + VAL_OFFSET);
	unsigned int value = *timer_value_addr;
	return value;
}

unsigned int enableVicInterrupt(int vic_base, int mask) {
	unsigned int* vic_enable_addr = (unsigned int*) (vic_base + VIC_IN_EN_OFFSET);
	*vic_enable_addr = (*vic_enable_addr) & mask;
	return *vic_enable_addr;
}

// Prototype for the user program main function
void umain();

// #define DEV_MAIN
#ifdef DEV_MAIN
void clockTick() {
	unsigned int time = 0;
	int tid = MyTid();
	while(1) {
		AwaitEvent(EVENT_TIME_ELAP, NULL, 0);
		time++;
		bwprintf(COM2, "%d: %ds\n", tid, time);
	}
}

void umain() {
	createIdleTask();
	bwprintf(COM2, "Hello World!\n");
	int i = 5;
	for(; i > 0; i--) {
		Create(5, clockTick);
	}
}
#endif

int main() {
	/* Initialize hardware */
	// Turn off interrupt
	asm("msr 	CPSR_c, #0xd3");

	// Turn off FIFO
	bwsetfifo(COM2, OFF);

	// Setup timer
	setTimerLoadValue(TIMER3_BASE, TIMER_TICK_SIZE);
	setTimerControl(TIMER3_BASE, TRUE, TRUE, FALSE);

	// Enable timer interrupt
	enableVicInterrupt(VIC2_BASE, VIC_TIMER3_MASK);

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

	/* Create first task with highest priority */
	Task *first_task = createTask(&flist, 0, umain);
	insertTask(&tlist, first_task);

	/* Main syscall handling loop */
	while(1){
		// If no more task to run, break
		if(!scheduleNextTask(&tlist)) break;
		UserTrapframe* user_sp = (UserTrapframe *)tlist.curtask->current_sp;
		DEBUG(DB_TASK, "| TASK:\tEXITING SP: 0x%x SPSR: 0x%x ResumePoint: 0x%x\n", user_sp, user_sp->spsr, user_sp->resume_point);
		// Exit kernel to let user program to execute
		kernelExit(tlist.curtask->current_sp);

		asm("mov r1, %0"
		    :
		    :"r"(&global)
		    :"r0", "r2", "r3"
		    );
		asm("bl	handlerRedirection(PLT)");
	}

	/* Turm off timer */
	setTimerControl(TIMER3_BASE, FALSE, FALSE, FALSE);

	return 0;
}
