#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>
#include <train.h>

void umain() {
	int tid = -1;

	tid = Create(5, nameserver);
	tid = Create(3, clockserver);
	tid = CreateWithArgs(3, comserver, COM1, 0, 0, 0);
	tid = CreateWithArgs(3, comserver, COM2, 0, 0, 0);
	createIdleTask();

	tid = Create(5, trainBootstrap);
}
