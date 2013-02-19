#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

void com1test() {
	char train_id = 35;
	int ch;
	char speed = 0;
	while (1) {
		speed = 30 - speed;
		iprintf("Press any key to continue...");
		ch = Getc(COM2);
		iprintf("S: %d\n", speed);
		Putc(COM1, speed);
		Putc(COM1, train_id);
		Delay(100);
	}
}

void com2test() {
	int ch;
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
	tid = Create(3, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	comserver_id = COM2;
	tid = Create(4, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	// tid = Create(7, com2test);
	tid = Create(7, com1test);
	createIdleTask();
}
