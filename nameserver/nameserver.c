#include <klib.h>
#include <unistd.h>
#include <kern/types.h>

#define NS_TID 1
#define NS_REG_MAX 16
#define NS_NAME_LEN_MAX 12
#define NS_QUERY_TYPE_REG 'R'
#define NS_QUERY_TYPE_WHO 'W'

typedef struct ns_reg {
	char name[NS_NAME_LEN_MAX];
	int tid;
} NSRegEntry;

typedef struct ns_query {
	char type;
	char *name;
	size_t nameLen;
} NSQuery;

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
	NSQuery query;
	char replymsg[5];
	replymsg[4] = '\0';

	NSRegEntry table[NS_REG_MAX];
	DEBUG(DB_NS, "Size of NSRegEntry is %d", sizeof(NSRegEntry));
	int i = 0;
	for (i = 0; i < NS_NAME_LEN_MAX; i++) {
		table[i].name[0] = '\0';
	}
	int ns_counter = 0;

	while (1) {
		ret = Receive(&tid, (char *)(&query), sizeof(NSQuery));

		// If failed to receive a request, continue for next request
		if (ret <= 0) {
			DEBUG(DB_NS, "| NS:\tReceived bad query\n");
			continue;
		}

		// Find the query name first
		i = findTidFor(table, query.name, ns_counter);
		DEBUG(DB_NS, "| NS:\tTid found for name %s is %d\n", query.name, table[i].tid);

		// If is a Register
		if (query.type == 'R') {
			DEBUG(DB_NS, "| NS:\tRegistering name %s to tid %d\n", query.name, tid);
			// If found matchs, it's a duplicate, overwrite it
			if (i != -1) {
				table[i].tid = tid;
			}
			// Else save as new one
			else {
				table[ns_counter].tid = tid;
				strncpy(table[ns_counter].name, query.name, NS_NAME_LEN_MAX);
				ns_counter++;	// Increament the counter
			}
			ret = Reply(tid, replymsg, 0);
		}
		// If is a WhoIs
		else if (query.type == 'W') {
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
	// assert(name[0] == 'R', "RegisterAs: Name must start with R");

	unsigned int nameLen = strlen(name);
	assert(nameLen > 0 && nameLen < NS_NAME_LEN_MAX, "RegisterAs: name too long");

	NSQuery query;
	query.type = NS_QUERY_TYPE_REG;
	query.name = name;
	query.nameLen = nameLen;
	char reply[1];

	if (Send(NS_TID, (char *)(&query), sizeof(NSQuery), reply, 1) < 0) {
		return -1;
	}
	return 0;
}

int WhoIs( char *name ) {
	assert(name != NULL, "WhoIs: NULL name pointer");
	// assert(name[0] == 'W', "WhoIs: Name must start with W");

	unsigned int nameLen = strlen(name);
	assert(nameLen > 0 && nameLen < NS_NAME_LEN_MAX, "RegisterAs: name too long");

	NSQuery query;
	query.type = NS_QUERY_TYPE_WHO;
	query.name = name;
	query.nameLen = nameLen;

	char reply[6];
	int retval = -1;
	if (Send(NS_TID, (char *)(&query), sizeof(NSQuery), reply, 6) < 0) {
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
