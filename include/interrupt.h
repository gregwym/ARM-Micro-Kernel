#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <intern/trapframe.h>
#include <task.h>

typedef enum uart_stat_type {
	US_MI,
	US_RI,
	US_TI,
	US_MI_CTS_TRUE,
	US_RI_OE,
	US_RI_FE,
	US_RI_NO_WAITING,
	US_TI_WAIT_CTS,
	US_TI_NO_WAITING,
	US_TOTAL
} UartStatType;

typedef struct uart_stat {
	unsigned int	counts[US_TOTAL];
} UartStat;

typedef struct kernel_global {
	ReadyQueue	*ready_queue;
	FreeList	*free_list;
	BlockedList	*receive_blocked_lists;
	BlockedList	*event_blocked_lists;
	MsgBuffer	*msg_array;
	Task		*task_array;
	int			uart1WaitingCTS;
	UartStat	uart_stat[2];
} KernelGlobal;

void kernelGlobalInitial(KernelGlobal *global);
void printStat(KernelGlobal *global);

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp);
int syscallHandler(void **parameters, KernelGlobal *global);
void irqHandler(KernelGlobal *global);

#define STAT_UART(kernel_global, uart_base, type) \
	(kernel_global->uart_stat[(uart_base == UART1_BASE ? 0 : 1)].counts[type]++)

#endif // _INTERRUPT_H_
