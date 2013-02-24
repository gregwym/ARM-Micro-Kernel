#include <interrupt.h>
#include <klib.h>

void kernelGlobalInitial(KernelGlobal *global) {
	global->uart1_waiting_cts = FALSE;

	int i = 0, j = 0;
	for(i = 0; i < 2; i++) {
		for(j = 0; j < US_TOTAL; j++) {
			global->uart_stat[i].counts[j] = 0;
		}
	}
}

void printStat(KernelGlobal *global) {
	const char *uart_stat_name[US_TOTAL];
	uart_stat_name[US_MI] = "US_MI";
	uart_stat_name[US_RI] = "US_RI";
	uart_stat_name[US_TI] = "US_TI";
	uart_stat_name[US_MI_CTS_TRUE] = "US_MI_CTS_TRUE";
	uart_stat_name[US_RI_OE] = "US_RI_OE";
	uart_stat_name[US_RI_FE] = "US_RI_FE";
	uart_stat_name[US_RI_NO_WAITING] = "US_RI_NO_WAITING";
	uart_stat_name[US_TI_WAIT_CTS] = "US_TI_WAIT_CTS";
	uart_stat_name[US_TI_NO_WAITING] = "US_TI_NO_WAITING";

	int i = 0, j = 0;
	for(i = 0; i < 2; i++) {
		bwprintf(COM2, "UART%d: \n", i);
		for(j = 0; j < US_TOTAL; j++) {
			bwprintf(COM2, "%s: %d\n", uart_stat_name[j], global->uart_stat[i].counts[j]);
		}
	}
}

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp) {
	global->ready_queue->curtask->current_sp = user_sp;

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
