#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

void test() {
	int ch;
	while (1) {
		ch = Getc(2);
		Putc(2, (char)ch);
	}
}

void umain() {
	int tid = -1;
	int comserver_id = COM1;
	
	tid = Create(5, nameserver);
	
	tid = Create(3, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);
	
	tid = Create(4, comserver);
	comserver_id = COM2;
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);
	
	tid = Create(7, test);
	createIdleTask();
}
