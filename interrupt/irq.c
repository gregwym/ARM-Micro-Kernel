#include <interrupt.h>
#include <klib.h>
#include <kern/unistd.h>

void timerIrqHandler() {

}

void irqHandler(KernelGlobal *global) {
	unsigned int *vic1_irq_st_addr = (unsigned int*) (VIC1_BASE + VIC_IRQ_ST_OFFSET);
	unsigned int *vic2_irq_st_addr = (unsigned int*) (VIC2_BASE + VIC_IRQ_ST_OFFSET);
	unsigned int *timer3_clr_addr = (unsigned int*) (TIMER3_BASE + CLR_OFFSET);

	DEBUG(DB_IRQ, "| IRQ:\tCaught,\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);
	*timer3_clr_addr = 0xff;
	DEBUG(DB_IRQ, "| IRQ:\tCleared,\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);

	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	TaskList	*task_list = global->task_list;
	MsgBuffer	*msg_array = global->msg_array;
	int tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);

	while(tid != -1) {
		msg_array[tid].event = NULL;
		insertTask(task_list, &(task_array[tid % TASK_MAX]));
		DEBUG(DB_IRQ, "| IRQ:\tTid %d resumed from EventBlocked\n", tid);
		tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);
	}
}
