#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

void test() {
	int ch;
	iprintf(COM2, "Hello World\n");
	while (1) {
		ch = Getc(COM2);
		// Putc(COM2, (char)ch);
	}
}

void umain() {
	int tid = -1;

	tid = Create(5, nameserver);

	tid = Create(3, clockserver);

	int comserver_id = COM1;
	// tid = Create(3, comserver);
	// Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	comserver_id = COM2;
	tid = Create(4, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	tid = Create(7, test);
	createIdleTask();
}
