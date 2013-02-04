#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>


void timeclient_p3() {
	int counter = 0;
	while (counter < 20) {
		counter++;
		Delay(10);
		DEBUG(DB_CS, "| CS:\tp3 delayed 10 secs\n");
	}
}

void timeclient_p4() {
	int counter = 0;
	while (counter < 9) {
		counter++;
		Delay(23);
		DEBUG(DB_CS, "| CS:\tp4 delayed 23 secs\n");
	}
}

void timeclient_p5() {
	int counter = 0;
	while (counter < 6) {
		counter++;
		Delay(33);
		DEBUG(DB_CS, "| CS:\tp5 delayed 33 secs\n");
	}
}

void timeclient_p6() {
	int counter = 0;
	while (counter < 3) {
		counter++;
		Delay(71);
		DEBUG(DB_CS, "| CS:\tp6 delayed 71 secs\n");
	}
}

void umain() {
	int tid;
	tid = Create(4, nameserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(2, clockserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(5, timeclient_p3);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(6, timeclient_p4);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(7, timeclient_p5);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(8, timeclient_p6);
	bwprintf(COM2, "Created: %d\n", tid);
	createIdleTask();
}


