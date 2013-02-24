#include <interrupt.h>
#include <klib.h>
#include <kern/unistd.h>

#include <ts7200.h>

void timerIrqHandler(KernelGlobal *global) {
	// Clear timer1's interrupt
	unsigned int *timer1_clr_addr = (unsigned int*) (TIMER1_BASE + CLR_OFFSET);
	*timer1_clr_addr = 0xff;

	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	ReadyQueue	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	// Dequeue event from the event_blocked_list
	int tid = dequeueBlockedList(event_blocked_lists, EVENT_TIME_ELAP);

	// If there is any blocked task
	if(tid == -1) {
		global->timer_stat.missed_tick++;
	}
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

void uartIrqHandler(KernelGlobal *global, unsigned int uart_base) {
	// Fetch global variables
	BlockedList *event_blocked_lists = global->event_blocked_lists;
	Task	 	*task_array = global->task_array;
	ReadyQueue	*ready_queue = global->ready_queue;
	MsgBuffer	*msg_array = global->msg_array;

	char *uart_data_addr = (char *) (uart_base + UART_DATA_OFFSET);
	unsigned int *uart_flag_addr = (unsigned int *) (uart_base + UART_FLAG_OFFSET);
	unsigned int *uart_intr_addr = (unsigned int *) (uart_base + UART_INTR_OFFSET);
	unsigned int *uart_rsr_addr = (unsigned int *) (uart_base + UART_RSR_OFFSET);
	int tx_event = uart_base == UART1_BASE ? EVENT_COM1_TX : EVENT_COM2_TX;
	int rx_event = uart_base == UART1_BASE ? EVENT_COM1_RX : EVENT_COM2_RX;

	// If is a modem interrupt
	if((*uart_intr_addr) & MIS_MASK) {
		STAT_UART(global, uart_base, US_MI);
		assert(uart_base == UART1_BASE, "Got MIS IRQ but not for UART1");
		DEBUG(DB_IRQ, "| IRQ:\tModem IRQ with flag 0x%x\n", *uart_flag_addr);
		// If cts is asserted
		if((*uart_flag_addr) & CTS_MASK) {
			STAT_UART(global, uart_base, US_MI_CTS_TRUE);
			// Stop waiting for cts, enable TX IRQ
			global->uart1_waiting_cts = FALSE;
			if(event_blocked_lists[tx_event].head != NULL)
				setUARTControlBit(uart_base, TIEN_MASK, TRUE);

			DEBUG(DB_IRQ, "| IRQ:\tCOM1 Clear to send\n");
		}
		// Clear modem interrupt
		setRegister(uart_base, UART_INTR_OFFSET, FALSE);
	}
	// If is receive buffer full
	else if((*uart_intr_addr) & RIS_MASK) {
		STAT_UART(global, uart_base, US_RI);
		assert((*uart_flag_addr) & RXFF_MASK, "Got RX IRQ but RX FIFO not full");
		if((*uart_rsr_addr) & LOW_4_MASK) {
			if((*uart_rsr_addr) & OE_MASK) STAT_UART(global, uart_base, US_RI_OE);
			if((*uart_rsr_addr) & FE_MASK) STAT_UART(global, uart_base, US_RI_FE);

			assert((*uart_rsr_addr) & OE_MASK, "Overrun");
			assert((*uart_rsr_addr) & FE_MASK, "Frame Error");

			*uart_rsr_addr = 0xff;
		}

		// Find COM1_RX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, rx_event);
		if(tid == -1) {
			STAT_UART(global, uart_base, US_RI_NO_WAITING);
			// Turn off RX interrupt if no waiting receive notifier
			setUARTControlBit(uart_base, RIEN_MASK, FALSE);
			DEBUG(DB_IRQ, "| IRQ:\tTX IRQ waiting for receiver\n");
			return;
		}

		// Check task state and event msg len
		Task *task = &(task_array[tid % TASK_MAX]);
		assert(task->state == EventBlocked, "Task in event_blocked_lists was not EventBlocked");
		assert(msg_array[tid].eventlen == sizeof(char), "RX event msg len is not sizeof(char)");

		// Fetch the char
		// Side effect: clear the RX IRQ
		char data = (*uart_data_addr) & DATA_MASK;

		// Copy the event msg in
		memcpy(msg_array[tid].event, &data, sizeof(char));
		msg_array[tid].event = NULL;

		// Put the task back to the ready queue
		insertTask(ready_queue, task);
		DEBUG(DB_IRQ, "| IRQ:\tReceive 0x%x to task %d\n", data, tid);
	}
	// If is none above, and is waiting for cts
	else if(uart_base == UART1_BASE && global->uart1_waiting_cts) {
		STAT_UART(global, uart_base, US_TI_WAIT_CTS);
		// Disable TX IRQ
		setUARTControlBit(uart_base, TIEN_MASK, FALSE);
		DEBUG(DB_IRQ, "| IRQ:\tTX IRQ waiting for CTS\n");
	}
	// If is transmit buffer empty
	else if((*uart_intr_addr) & TIS_MASK) {
		STAT_UART(global, uart_base, US_TI);
		assert((*uart_flag_addr) & TXFE_MASK, "Got TX IRQ but TX FIFO not empty");
		assert((uart_base != UART1_BASE) || ((*uart_flag_addr) & CTS_MASK),
		       "Got TX IRQ but CTS is not assertted");

		// Find COM1_TX blocked task
		int tid = dequeueBlockedList(event_blocked_lists, tx_event);
		if(tid == -1) {
			STAT_UART(global, uart_base, US_TI_NO_WAITING);
			// Turn off TX interrupt if no more waiting transmit notifier
			setUARTControlBit(uart_base, TIEN_MASK, FALSE);
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
		*uart_data_addr = data;

		// If it is UART1
		if(uart_base == UART1_BASE) {
			// Start waiting for cts and disable TX IRQ
			global->uart1_waiting_cts = TRUE;
		}
		setUARTControlBit(uart_base, TIEN_MASK, FALSE);

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

	// If IRQ from Timer1
	if ((*vic1_irq_st_addr) & VIC_TIMER1_MASK) {
		timerIrqHandler(global);
		assert(!((*vic1_irq_st_addr) & VIC_TIMER1_MASK), "Failed to clear the timer interrupt");
	}
	else if ((*vic2_irq_st_addr) & VIC_UART2_MASK) {
		uartIrqHandler(global, UART2_BASE);
	}
	else if ((*vic2_irq_st_addr) & VIC_UART1_MASK) {
		uartIrqHandler(global, UART1_BASE);
	}
	else {
		assert(0, "Unknown IRQ type");
	}

	DEBUG(DB_IRQ, "| IRQ:\tCleared\tVIC1: 0x%x VIC2: 0x%x\n", *vic1_irq_st_addr, *vic2_irq_st_addr);
}
