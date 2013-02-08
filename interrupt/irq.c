#include <interrupt.h>
#include <klib.h>
#include <kern/unistd.h>

void timerIrqHandler(KernelGlobal *global) {
	// Clear timer3's interrupt
	unsigned int *timer3_clr_addr = (unsigned int*) (TIMER3_BASE + CLR_OFFSET);
	*timer3_clr_addr = 0xff;

	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	TaskList	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	// Dequeue event from the event_blocked_list
	int tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);

	// If there is any blocked task
	while(tid != -1) {
		// Change task states and clear MsgBuffer
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		task->state = Ready;
		msg_array[tid].event = NULL;

		// Insert the task back to priority queue
		insertTask(ready_queue, task);
		DEBUG(DB_IRQ, "| IRQ:\tTid %d resumed from EventBlocked\n", tid);

		// Try to dequeue next blocked task
		tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);
	}
}

void irqHandler(KernelGlobal *global) {

	// Access VICs' IRQ Status Registers
	unsigned int *vic1_irq_st_addr = (unsigned int*) (VIC1_BASE + VIC_IRQ_ST_OFFSET);
	unsigned int *vic2_irq_st_addr = (unsigned int*) (VIC2_BASE + VIC_IRQ_ST_OFFSET);
	DEBUG(DB_IRQ, "| IRQ:\tCaught\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);

	// If IRQ from Timer3
	if ((*vic2_irq_st_addr) & VIC_TIMER3_MASK) {
		timerIrqHandler(global);
		assert(!((*vic2_irq_st_addr) & VIC_TIMER3_MASK), "Failed to clear the timer interrupt");
	}
	else {
		assert(0, "Unknown IRQ type");
	}

	DEBUG(DB_IRQ, "| IRQ:\tCleared\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);
}
