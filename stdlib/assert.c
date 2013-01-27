#include <bwio.h>
#include <stdlib.h>

void assert(int boolean, char * msg) {
	if (!boolean) {
		bwprintf(COM2, "\e[31mError: %s \e[m\n", msg);
	}
}

