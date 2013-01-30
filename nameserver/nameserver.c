#include <klib.h>
#include <unistd.h>
#include <kern/types.h>

#define NS_TID 1
#define NS_REG_MAX 16
#define NS_NAME_LEN_MAX 12
#define NS_QUERY_TYPE_REG 'R'
#define NS_QUERY_TYPE_WHO 'W'
#define NS_REPLY_TYPE_FND 'F'
#define NS_REPLY_TYPE_NOT 'N'

typedef struct ns_reg {
	char name[NS_NAME_LEN_MAX];
	int tid;
} NSRegEntry;

typedef struct ns_query {
	char type;
	char *name;
} NSQuery;

typedef struct ns_reply {
	char result;
	int tid;
} NSReply;

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
	DEBUG(DB_NS, "| NS:\tBooting\n");
	int ret = -1;
	int tid = 0;
	NSQuery query;
	NSReply reply;

	NSRegEntry table[NS_REG_MAX];
	int i = 0;
	for (i = 0; i < NS_REG_MAX; i++) {
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
		i = findTidFor(table, query.name, NS_REG_MAX);
		DEBUG(DB_NS, "| NS:\tTid found for name %s is at index %d\n", query.name, i);

		// If is a Register
		if (query.type == NS_QUERY_TYPE_REG) {
			DEBUG(DB_NS, "| NS:\tRegistering name %s with tid %d\n", query.name, tid);
			// If found matchs, it's a duplicate, overwrite it
			if (i != -1) {
				table[i].tid = tid;
			}
			// Else save as new one
			else {
				table[ns_counter].tid = tid;
				strncpy(table[ns_counter].name, query.name, NS_NAME_LEN_MAX);
				ns_counter = (ns_counter + 1) % NS_REG_MAX;	// Increament the counter
			}
			ret = Reply(tid, NULL, 0);
		}
		// If is a WhoIs
		else if (query.type == NS_QUERY_TYPE_WHO) {
			// If found nothing match, reply 'N'
			if (i < 0) {
				reply.result = NS_REPLY_TYPE_NOT;
			}
			// Else, reply 'F' and found tid
			else {
				reply.result = NS_REPLY_TYPE_FND;
				reply.tid = table[i].tid;
				DEBUG(DB_NS, "| NS:\tReplying name %s with tid %d\n", query.name, reply.tid);
			}
			ret = Reply(tid, (char *)(&reply), sizeof(NSReply));
		}
		else {
			DEBUG(DB_NS, "| NS:\tInvalid request type, should never be here!\n");
			break;
		}
	}
}

/* Nameserver syscall wrappers */
int RegisterAs( char *name ) {
	assert(name != NULL, "| RegisterAs:\tNULL name pointer");
	// assert(name[0] == 'R', "| RegisterAs:\tName must start with R");

	unsigned int nameLen = strlen(name);
	assert(nameLen > 0 && nameLen < NS_NAME_LEN_MAX, "| RegisterAs:\tname too long");

	NSQuery query;
	query.type = NS_QUERY_TYPE_REG;
	query.name = name;

	if (Send(NS_TID, (char *)(&query), sizeof(NSQuery), NULL, 0) < 0) {
		return -1;
	}
	return 0;
}

int WhoIs( char *name ) {
	assert(name != NULL, "| WhoIs:\tNULL name pointer");
	// assert(name[0] == 'W', "| WhoIs:\tName must start with W");

	unsigned int nameLen = strlen(name);
	assert(nameLen > 0 && nameLen < NS_NAME_LEN_MAX, "| WhoIs:\tname too long");

	NSQuery query;
	query.type = NS_QUERY_TYPE_WHO;
	query.name = name;

	NSReply reply;
	if (Send(NS_TID, (char *)(&query), sizeof(NSQuery), (char *)(&reply), sizeof(NSReply)) < 0) {
		return -1;
	}

	if(reply.result == NS_REPLY_TYPE_NOT) {
		return -3;
	}
	assert(reply.result == NS_REPLY_TYPE_FND, "| WhoIs:\tGot unkown reply result");
	assert(reply.tid >= 0, "| WhoIs:\tGot negative tid");

	DEBUG(DB_NS, "| WhoIs:\tGot tid %d for name %s\n", reply.tid, name);

	return reply.tid;
}
