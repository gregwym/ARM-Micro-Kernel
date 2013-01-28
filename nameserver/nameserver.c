#include <klib.h>
#include <unistd.h>
#include <kern/types.h>

#define NAME_SERVER_TID 1


int RegisterAs( char *name ) {
	assert(name != NULL, "RegisterAs: NULL name pointer");
	assert(name[0] == 'R', "RegisterAs: Name must start with R");
	int namelen = strlen(name);
	char reply[1];

	if (Send(NAME_SERVER_TID, name, namelen + 1, reply, 1) < 0) {
		return -1;
	}
	return 0;
}

int WhoIs( char *name ) {
	assert(name != NULL, "WhoIs: NULL name pointer");
	assert(name[0] == 'W', "WhoIs: Name must start with W");
	int namelen = strlen(name);
	char reply[6];
	int retval = -1;
	if (Send(NAME_SERVER_TID, name, namelen + 1, reply, 6) < 0) {
		return -1;
	}

	if(reply[4] == 'N') {
		return -3;
	}
	assert(reply[4] == 'F', "WhoIs: Got unkown reply flag");

	copyBytes((char *) (&retval), reply);

	assert(retval >= 0, "WhoIs: Got negative tid");
	return retval;
}
