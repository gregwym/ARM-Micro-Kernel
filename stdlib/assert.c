#include <ts7200.h>
#include <bwio.h>
#include <stdlib.h>

void assert(int boolean, char * msg) {
	if (boolean) {
		bwprintf(COM2, "\e[31;mError: %s \e[m\n", msg);
	}
}

void debug(int debug, char *msg) {
	if (debug <= DEBUG_LVL) {
		bwprintf(COM2, "\e[33;mError: %s \e[m\n", msg);
	}
}
