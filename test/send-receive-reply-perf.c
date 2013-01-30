#include <kern/ts7200.h>
#include <klib.h>
#include <unistd.h>

#define FALSE 0x00000000
#define TRUE 0xffffffff

unsigned int setTimerControl(int timer_base, unsigned int enable, unsigned int mode, unsigned int clksel) {
	unsigned int* timer_control_addr = (unsigned int*) (timer_base + CRTL_OFFSET);
	unsigned int control_value = (ENABLE_MASK & enable) | (MODE_MASK & mode) | (CLKSEL_MASK & clksel) ;

	*timer_control_addr = control_value;
	return *timer_control_addr;
}

unsigned int getTimerValue(int timer_base) {
	unsigned int* timer_value_addr = (unsigned int*) (timer_base + VAL_OFFSET);
	unsigned int value = *timer_value_addr;
	return value;
}

void sender() {
	time_t start = getTimerValue(TIMER3_BASE);
	time_t end = 0;
	char fourBytes[4] = "abc";
	char sixtyFourBytes[64] = "123456789012345678901234567890123456789012345678901234567890123";
	char fourBytesReply[4];
	char sixtyFourBytesReply[64];

	start = getTimerValue(TIMER3_BASE);
	bwprintf(COM2, "Sender start to send 4bytes \'%s\' at %u\n", fourBytes, start);
	start = getTimerValue(TIMER3_BASE);
	Send(2, fourBytes, 4, fourBytesReply, 4);
	end = getTimerValue(TIMER3_BASE);
	bwprintf(COM2, "Sender receive reply: \'%s\' at %u, %u tick elapsed\n", fourBytesReply, end, start - end);

	start = getTimerValue(TIMER3_BASE);
	bwprintf(COM2, "Sender start to send 64bytes \'%s\' at %u\n", sixtyFourBytes, start);
	start = getTimerValue(TIMER3_BASE);
	Send(2, sixtyFourBytes, 64, sixtyFourBytesReply, 64);
	end = getTimerValue(TIMER3_BASE);
	bwprintf(COM2, "Sender receive reply: \'%s\' at %u, %u tick elapsed\n", sixtyFourBytesReply, end, start - end);
}

void receiver() {
	char fourBytes[4];
	char sixtyFourBytes[64];
	int tid;

	Receive(&tid, fourBytes, 4);
	Reply(tid, fourBytes, 4);
	Receive(&tid, sixtyFourBytes, 64);
	Reply(tid, sixtyFourBytes, 64);
}

void umain() {
	/* Initialize Timer: Enable Timer3 with free running mode and 508kHz clock */
	setTimerControl(TIMER3_BASE, TRUE, FALSE, TRUE);

	int tid;
	tid = Create(1, sender);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(1, receiver);
	bwprintf(COM2, "Created: %d\n", tid);
}
