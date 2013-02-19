#include <interrupt.h>
#include <klib.h>
#include <kern/unistd.h>

#include <ts7200.h>

void timerIrqHandler(KernelGlobal *global) {
	// Clear timer3's interrupt
	unsigned int *timer3_clr_addr = (unsigned int*) (TIMER3_BASE + CLR_OFFSET);
	*timer3_clr_addr = 0xff;

	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	ReadyQueue	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	// Dequeue event from the event_blocked_list
	int tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);

	// If there is any blocked task
	while(tid != -1) {
		// Change task states and clear MsgBuffer
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		msg_array[tid].event = NULL;

		// Insert the task back to priority queue
		insertTask(ready_queue, task);
		DEBUG(DB_IRQ, "| IRQ:\tTid %d resumed from EventBlocked\n", tid);

		// Try to dequeue next blocked task
		tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);
	}
}

void uart2IrqHandler(KernelGlobal *global) {
	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	ReadyQueue	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	char *uart2_data_addr = (char *) (UART2_BASE + UART_DATA_OFFSET);
	unsigned int *uart2_flag_addr = (unsigned int *) (UART2_BASE + UART_FLAG_OFFSET);
	unsigned int *uart2_intr_addr = (unsigned int *) (UART2_BASE + UART_INTR_OFFSET);

	// If is receive buffer full
	if((*uart2_intr_addr) & RIS_MASK) {
		assert((*uart2_flag_addr) & RXFF_MASK, "Got RX IRQ but RX FIFO not full");

		// Find COM2_RX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, EVENT_COM2_RX);
		if(tid == -1) {
			// Turn off RX interrupt if no waiting receive notifier
			setUARTControlBit(UART2_BASE, RIEN_MASK, FALSE);
			return;
		}

		// Check task state and event msg len
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		assert(msg_array[tid].eventlen == sizeof(char), "RX event msg len is not sizeof(char)");

		// Fetch the char
		// Side effect: clear the RX IRQ
		char data = (*uart2_data_addr) & DATA_MASK;

		// Copy the event msg in
		memcpy(msg_array[tid].event, &data, sizeof(char));
		msg_array[tid].event = NULL;

		// Put the task back to the ready queue
		insertTask(ready_queue, task);
	}

	// If is transmit buffer empty
	else if((*uart2_intr_addr) & TIS_MASK) {
		assert((*uart2_flag_addr) & TXFE_MASK, "Got TX IRQ but TX FIFO not empty");

		// Find COM2_TX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, EVENT_COM2_TX);
		if(tid == -1) {
			// Turn off TX interrupt if no more waiting transmit notifier
			setUARTControlBit(UART2_BASE, TIEN_MASK, FALSE);
			return;
		}

		// Check task state and event msg len
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		assert(msg_array[tid].eventlen == sizeof(char), "TX event msg len is not sizeof(char)");

		// Copy the event msg out
		char data;
		memcpy(&data, msg_array[tid].event, sizeof(char));
		msg_array[tid].event = NULL;

		// Send the char
		// Side effect: clear the TX IRQ
		*uart2_data_addr = data;

		// Put the task back to the ready queue
		insertTask(ready_queue, task);
	}
}

void uart1IrqHandler(KernelGlobal *global) {
	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	ReadyQueue	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	char *uart1_data_addr = (char *) (UART1_BASE + UART_DATA_OFFSET);
	unsigned int *uart1_flag_addr = (unsigned int *) (UART1_BASE + UART_FLAG_OFFSET);
	unsigned int *uart1_intr_addr = (unsigned int *) (UART1_BASE + UART_INTR_OFFSET);

	// If is a modem interrupt
	if((*uart1_intr_addr) & MIS_MASK) {
		DEBUG(DB_IRQ, "| IRQ:\tModem IRQ with flag 0x%x\n", *uart1_flag_addr);
		// If cts is asserted
		if((*uart1_flag_addr) & CTS_MASK) {
			// Stop waiting for cts, enable TX IRQ
			global->uart1WaitingCTS = FALSE;
			setUARTControlBit(UART1_BASE, TIEN_MASK, TRUE);
			DEBUG(DB_IRQ, "| IRQ:\tCOM1 Clear to send\n");
		}
		// Clear modem interrupt
		setRegister(UART1_BASE, UART_INTR_OFFSET, FALSE);
	}
	// If is receive buffer full
	else if((*uart1_intr_addr) & RIS_MASK) {
		assert((*uart1_flag_addr) & RXFF_MASK, "Got RX IRQ but RX FIFO not full");

		// Find COM1_RX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, EVENT_COM1_RX);
		if(tid == -1) {
			// Turn off RX interrupt if no waiting receive notifier
			setUARTControlBit(UART1_BASE, RIEN_MASK, FALSE);
			DEBUG(DB_IRQ, "| IRQ:\tTX IRQ waiting for receiver\n");
			return;
		}

		// Check task state and event msg len
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		assert(msg_array[tid].eventlen == sizeof(char), "RX event msg len is not sizeof(char)");

		// Fetch the char
		// Side effect: clear the RX IRQ
		char data = (*uart1_data_addr) & DATA_MASK;

		// Copy the event msg in
		memcpy(msg_array[tid].event, &data, sizeof(char));
		msg_array[tid].event = NULL;

		// Put the task back to the ready queue
		insertTask(ready_queue, task);
		DEBUG(DB_IRQ, "| IRQ:\tReceive 0x%x to task %d\n", data, tid);
	}
	// If is none above, and is waiting for cts
	else if(global->uart1WaitingCTS) {
		// Disable TX IRQ
		setUARTControlBit(UART1_BASE, TIEN_MASK, FALSE);
		DEBUG(DB_IRQ, "| IRQ:\tTX IRQ waiting for CTS\n");
	}
	// If is transmit buffer empty
	else if((*uart1_intr_addr) & TIS_MASK) {
		assert((*uart1_flag_addr) & TXFE_MASK, "Got TX IRQ but TX FIFO not empty");
		assert((*uart1_flag_addr) & CTS_MASK, "Got TX IRQ but CTS is not assertted");

		// Find COM1_TX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, EVENT_COM1_TX);
		if(tid == -1) {
			// Turn off TX interrupt if no more waiting transmit notifier
			setUARTControlBit(UART1_BASE, TIEN_MASK, FALSE);
			DEBUG(DB_IRQ, "| IRQ:\tTX IRQ waiting for sender\n");
			return;
		}

		// Check task state and event msg len
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		assert(msg_array[tid].eventlen == sizeof(char), "TX event msg len is not sizeof(char)");

		// Copy the event msg out
		char data;
		memcpy(&data, msg_array[tid].event, sizeof(char));
		msg_array[tid].event = NULL;

		// Send the char
		// Side effect: clear the TX IRQ
		*uart1_data_addr = data;

		// Start waiting for cts and disable TX IRQ
		global->uart1WaitingCTS = TRUE;
		setUARTControlBit(UART1_BASE, TIEN_MASK, FALSE);

		// Put the task back to the ready queue
		insertTask(ready_queue, task);
		DEBUG(DB_IRQ, "| IRQ:\tSent 0x%x from task %d\n", data, tid);
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
	else if ((*vic2_irq_st_addr) & VIC_UART2_MASK) {
		uart2IrqHandler(global);
	}
	else if ((*vic2_irq_st_addr) & VIC_UART1_MASK) {
		uart1IrqHandler(global);
	}
	else {
		assert(0, "Unknown IRQ type");
	}

	DEBUG(DB_IRQ, "| IRQ:\tCleared\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);
}
