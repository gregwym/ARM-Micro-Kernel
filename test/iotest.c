#include <klib.h>
#include <unistd.h>
#include <services.h>

void test() {
	int ch;
	while (1) {
		ch = Getc(2);
		Putc(2, (char)ch);
	}
}

void umain() {
	int tid = -1;

	tid = Create(4, nameserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, comserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, comserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(5, test);
	bwprintf(COM2, "Created: %d\n", tid);
	createIdleTask();
}