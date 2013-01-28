#include <klib.h>
#include <unistd.h>

#define NAME_SERVER_TID 1


int RegisterAs( char *name ) {
	int msglen = strlen(name);
	char reply[6];
	int retval = -1;
	if (Send(NAME_SERVER_TID, name, msglen, reply, 6) >= 0) {

		// bwprintf(COM2, "reply msg size: %d\n", strlen(reply));
		char *val = (char *) &retval;
		val[0] = reply[0];
		val[1] = reply[1];
		val[2] = reply[2];
		val[3] = reply[3];
		if (retval == 1) {
			return 0;
		} else {
			assert(retval != -1, "Forget R for register");
			return -2;
		}
	} else {
		return -1;
	}
}

int WhoIs( char *name ) {
	int msglen = strlen(name);
	char reply[5];
	int retval = -1;
	if (Send(NAME_SERVER_TID, name, msglen, reply, 5) >= 0) {
		char *val = (char *) &retval;
		val[0] = reply[0];
		val[1] = reply[1];
		val[2] = reply[2];
		val[3] = reply[3];
		if (retval == 0) {
			return 0;
		} else {
			assert(retval != -1, "Forget W for whois");
			return retval;
		}
	} else {
		return -1;
	}
}
