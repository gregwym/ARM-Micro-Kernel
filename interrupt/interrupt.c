#include <klib.h>
#include <intern/trapframe.h>

void handlerRedirection(void **parameters, KernelGlobal *global, UserTrapframe *user_sp) {
	global->task_list->curtask->current_sp = user_sp;

	// If is an IRQ
	if (parameters == NULL) {
		irqHandler(global);
	}
	else {
		syscallHandler(parameters, global, user_sp);
	}
}
