#include <ts7200.h>
#include <bwio.h>
#include <stdlib.h>

void assert(int boolean, char * msg) {
	if (boolean) {
		bwprintf(COM2, "\e[31;mError: %s \e[m\n", msg);
	}
}
