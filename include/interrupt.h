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

typedef struct task_stat {
	unsigned int	total_created;
	unsigned int	max_tid;
	unsigned int	boot_timestamp;
	unsigned int	last_resume_timestamp;
	unsigned int	active_time[TASK_MAX];
} TaskStat;

typedef struct timer_stat {
	unsigned int	missed_tick;
} TimerStat;

typedef struct kernel_global {
	ReadyQueue	*ready_queue;
	FreeList	*free_list;
	BlockedList	*receive_blocked_lists;
	BlockedList	*event_blocked_lists;
	MsgBuffer	*msg_array;
	Task		*task_array;
	int			uart1_waiting_cts;
	UartStat	uart_stat[2];
	TaskStat	task_stat;
	TimerStat	timer_stat;
} KernelGlobal;

void kernelGlobalInitial(KernelGlobal *global);
void printStat(KernelGlobal *global);

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp);
int syscallHandler(void **parameters, KernelGlobal *global);
void irqHandler(KernelGlobal *global);

#define STAT_UART(kernel_global, uart_base, type) \
	(kernel_global->uart_stat[(uart_base == UART1_BASE ? 0 : 1)].counts[type]++)
#define STAT_TASK_BEGIN(kernel_global) \
	((kernel_global)->task_stat.last_resume_timestamp = getTimerValue(TIMER3_BASE))
#define STAT_TASK_END(kernel_global, tid) \
	((kernel_global)->task_stat.active_time[tid]+=\
	 ((kernel_global)->task_stat.last_resume_timestamp - getTimerValue(TIMER3_BASE)))

#endif // _INTERRUPT_H_
