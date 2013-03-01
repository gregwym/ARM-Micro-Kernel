#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>
#include <train.h>

void umain() {
	int tid = -1;

	tid = Create(5, nameserver);
	tid = Create(3, clockserver);

	int comserver_id = COM1;
	tid = Create(3, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	comserver_id = COM2;
	tid = Create(3, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	tid = Create(6, trainBootstrap);
	createIdleTask();
}
