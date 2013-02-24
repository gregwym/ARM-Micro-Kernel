#include <interrupt.h>
#include <klib.h>
#include <ts7200.h>

void kernelGlobalInitial(KernelGlobal *global) {
	int i = 0, j = 0;

	global->task_stat.boot_timestamp = getTimerValue(TIMER3_BASE);
	for(i = 0; i < TASK_MAX; i++) {
		global->task_stat.active_time[i] = 0;
	}

	global->uart1_waiting_cts = FALSE;
	for(i = 0; i < 2; i++) {
		for(j = 0; j < US_TOTAL; j++) {
			global->uart_stat[i].counts[j] = 0;
		}
	}
}

void printStat(KernelGlobal *global) {
	int i = 0, j = 0;
	const char *uart_stat_name[US_TOTAL];
	uart_stat_name[US_MI] = "MI";
	uart_stat_name[US_RI] = "RI";
	uart_stat_name[US_TI] = "TI";
	uart_stat_name[US_MI_CTS_TRUE] = "MI_CTS_TRUE";
	uart_stat_name[US_RI_OE] = "RI_OE";
	uart_stat_name[US_RI_FE] = "RI_FE";
	uart_stat_name[US_RI_NO_WAITING] = "RI_NO_WAITING";
	uart_stat_name[US_TI_WAIT_CTS] = "TI_WAIT_CTS";
	uart_stat_name[US_TI_NO_WAITING] = "TI_NO_WAITING";

	bwprintf(COM2, "\e[15;1HTOTAL TIME: %u\n", global->task_stat.boot_timestamp - getTimerValue(TIMER3_BASE));
	for(i = 0; i < TASK_MAX; i++) {
		bwprintf(COM2, "TASK%d: %u\n", i, global->task_stat.active_time[i]);
	}

	for(i = 0; i < 2; i++) {
		bwprintf(COM2, "UART%d: \n", i);
		for(j = 0; j < US_TOTAL; j++) {
			bwprintf(COM2, "%s: %u\n", uart_stat_name[j], global->uart_stat[i].counts[j]);
		}
	}
}

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp) {
	global->ready_queue->curtask->current_sp = user_sp;

	STAT_TASK_END(global, global->ready_queue->curtask->tid);
	DEBUG(DB_TASK, "| TASK:\tHANDLER SP: 0x%x SPSR: 0x%x ResumePoint: 0x%x\n",
	      user_sp, user_sp->spsr, user_sp->resume_point);

	// If is an IRQ
	if (parameters == NULL) {
		irqHandler(global);
	}
	else {
		user_sp->r0 = syscallHandler(parameters, global);
	}
}
