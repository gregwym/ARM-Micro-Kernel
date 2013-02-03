#include <task.h>
#include <unistd.h>

void idleTask() {
	while(1);
}

void createIdleTask() {
	/* Create idle task with lowest priority */
	Create(TASK_PRIORITY_MAX - 1, idleTask);
}
