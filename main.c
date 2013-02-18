#include <klib.h>
#include <unistd.h>
#include <interrupt.h>
#include <ts7200.h>
#include <kern/md_const.h>
#include <services.h>

#define TIMER_TICK_SIZE	20

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

void sender() {
	char data[11] = "0123456789";
	int i = 0;

	while(++i) {
		Delay(100);
		AwaitEvent(EVENT_COM2_TX, &(data[i % 10]), sizeof(char));
	}
}

void receiver() {
	char data = '\0';

	while(1) {
		AwaitEvent(EVENT_COM2_RX, &data, sizeof(char));
		AwaitEvent(EVENT_COM2_TX, &data, sizeof(char));
	}
}

void umain() {
	Create(4, nameserver);
	Create(2, clockserver);
	Create(8, sender);
	Create(8, receiver);

	createIdleTask();
}
#endif

int main() {
	/* Initialize hardware */
	// Turn off interrupt
	asm("msr 	CPSR_c, #0xd3");

	// Setup UART2
	setUARTLineControl(UART2_BASE, 3, FALSE, FALSE, FALSE, FALSE, FALSE, 115200);
	setUARTControl(UART2_BASE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE);

	// Setup timer
	setTimerLoadValue(TIMER3_BASE, TIMER_TICK_SIZE);
	setTimerControl(TIMER3_BASE, TRUE, TRUE, FALSE);

	// Enable interrupt
	enableVicInterrupt(VIC2_BASE, VIC_TIMER3_MASK);
	enableVicInterrupt(VIC2_BASE, VIC_UART2_MASK);

	/* Initialize ReadyQueue and Task related data structures */
	ReadyQueue	ready_queue;
	Heap		task_heap;
	HeapNode	*task_heap_data[TASK_PRIORITY_MAX];
	HeapNode	task_heap_nodes[TASK_PRIORITY_MAX];
	FreeList	free_list;
	Task		task_array[TASK_MAX];
	TaskList	task_list[TASK_PRIORITY_MAX];
	char		stacks[TASK_MAX * TASK_STACK_SIZE];
	BlockedList	receive_blocked_lists[TASK_MAX];
	BlockedList	event_blocked_lists[EVENT_MAX];
	MsgBuffer	msg_array[TASK_MAX];

	taskArrayInitial(task_array, stacks);
	freeListInitial(&free_list, task_array);
	heapInitial(&task_heap, task_heap_data, TASK_PRIORITY_MAX);
	readyQueueInitial(&ready_queue, &task_heap, task_heap_nodes, task_list);
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
	global.ready_queue = &ready_queue;
	global.free_list = &free_list;
	global.receive_blocked_lists = receive_blocked_lists;
	global.event_blocked_lists = event_blocked_lists;
	global.msg_array = msg_array;
	global.task_array = task_array;

	/* Create first task with highest priority */
	Task *first_task = createTask(&free_list, 0, umain);
	insertTask(&ready_queue, first_task);

	/* Main syscall handling loop */
	while(1){
		// If no more task to run, break
		if(!scheduleNextTask(&ready_queue)) break;
		UserTrapframe* user_sp = (UserTrapframe *)ready_queue.curtask->current_sp;
		DEBUG(DB_TASK, "| TASK:\tEXITING SP: 0x%x SPSR: 0x%x ResumePoint: 0x%x\n", user_sp, user_sp->spsr, user_sp->resume_point);
		// Exit kernel to let user program to execute
		kernelExit(ready_queue.curtask->current_sp);

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
