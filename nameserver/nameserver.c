#include <klib.h>
#include <unistd.h>
#include <kern/types.h>

#define NS_TID 1
#define NS_REG_MAX 10
#define NS_NAME_LEN_MAX 10
#define NS_MSG_LEN_MAX 1 + NS_NAME_LEN_MAX

typedef struct ns_reg {
	char name[NS_NAME_LEN_MAX];
	int tid;
} NSRegEntry;

int findTidFor(NSRegEntry *table, char *name, int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (strcmp(name, table[i].name) == 0) {
			return i;
		}
	}
	return -1;
}

void nameserver() {
	DEBUG(DB_NS, "Nameserver booting\n");
	int ret = -1;
	int tid = 0;
	char msg[NS_MSG_LEN_MAX];
	char replymsg[5];
	replymsg[4] = '\0';

	NSRegEntry table[NS_REG_MAX];
	int i = 0;
	for (i = 0; i < NS_NAME_LEN_MAX; i++) {
		table[i].name[0] = '\0';
	}

	int ns_counter = 0;

	while (1) {
		DEBUG(DB_NS, "Nameserver call receive\n");
		ret = Receive(&tid, msg, NS_MSG_LEN_MAX);

		// If failed to receive a request, continue for next request
		if (ret <= 0) {
			continue;
		}

		// If is a Register
		if (msg[0] == 'R') {
			i = findTidFor(table, &(msg[1]), ns_counter);
			// If found matchs, it's a duplicate, overwrite it
			if (i != -1) {
				table[i].tid = tid;
				ret = Reply(tid, replymsg, 0);
			}
			// Else save as new one
			else {
				table[ns_counter].tid = tid;
				strncpy(table[ns_counter].name, &(msg[1]), 9);
				ret = Reply(tid, replymsg, 0);
				ns_counter++;	// Increament the counter
			}
		}
		// If is a WhoIs
		else if (msg[0] == 'W') {
			int i = findTidFor(table, &msg[1], ns_counter);
			// If found nothing match, reply 'N'
			if (i == -1) {
				replymsg[4] = 'N';
				ret = Reply(tid, replymsg, 5);
			}
			// Else, reply 'F' and found tid
			else {
				replymsg[4] = 'F';
				copyBytes(replymsg, (char *)(&table[i].tid));
				ret = Reply(tid, replymsg, 5);
			}
		}
		else {
			DEBUG(DB_NS, "Nameserver: Invalid request type, should never be here!\n");
			break;
		}
	}
}

/* Nameserver syscall wrappers */
int RegisterAs( char *name ) {
	assert(name != NULL, "RegisterAs: NULL name pointer");
	assert(name[0] == 'R', "RegisterAs: Name must start with R");
	int namelen = strlen(name);
	char reply[1];

	if (Send(NS_TID, name, namelen + 1, reply, 1) < 0) {
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
	if (Send(NS_TID, name, namelen + 1, reply, 6) < 0) {
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
