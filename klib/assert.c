#include <klib.h>

void _assert(char *pos, char *test, char *msg){
	/* print assertion message and abort */
	bwprintf(COM2, "\e[31m[%s] %s | \"%s\"\e[m\n", pos, test, msg);
	// abort();
}
