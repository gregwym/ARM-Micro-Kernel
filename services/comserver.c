#include <services.h>
#include <klib.h>
#include <unistd.h>

#define COM_BUFFER_SIZE		100
#define IO_QUERY_TYPE_GETC	0
#define IO_QUERY_TYPE_PUTC	1

#define COM1_REG_NAME 	"C1"
#define COM2_REG_NAME 	"C2"
#define COM_NAME_ARRAY_LEN	3

typedef struct putc_query {
	int type;
	char ch;
} PutcQuery;

typedef struct getc_query {
	int type;
} GetcQuery;

typedef union {
	int 		type;
	char		ch;
	PutcQuery	putcQuery;
	GetcQuery	getcQuery;
} IOMsg;

void com1SendNotifier() {
	char c1_name[] = COM1_REG_NAME;
	int com1_server_tid = WhoIs(c1_name);
	assert(com1_server_tid >= 0, "Notifier cannot find com1 server's tid");

	char reply[2];
	reply[1] = '\0';
	char send[] = "";

	while (1) {
		Send(com1_server_tid, send, 1, reply, 2);
		AwaitEvent(EVENT_COM1_TX , reply, sizeof(char));
	}
}

void com1ReceiveNotifier() {
	char c1_name[] = COM1_REG_NAME;
	int com1_server_tid = WhoIs(c1_name);
	assert(com1_server_tid >= 0, "Notifier cannot find com1 server's tid");

	char send[2];
	send[1] = '\0';

	while (1) {
		AwaitEvent(EVENT_COM1_RX , send, sizeof(char));
		Send(com1_server_tid, send, 2, NULL, 0);
	}
}

void com2SendNotifier() {
	char c2_name[] = COM2_REG_NAME;
	int com2_server_tid = WhoIs(c2_name);
	assert(com2_server_tid >= 0, "Notifier cannot find com2 server's tid");

	char reply[2];
	reply[1] = '\0';
	char send[] = "";

	while (1) {
		Send(com2_server_tid, send, 1, reply, 2);
		AwaitEvent(EVENT_COM2_TX , reply, sizeof(char));
	}
}

void com2ReceiveNotifier() {
	char c2_name[] = COM2_REG_NAME;
	int com2_server_tid = WhoIs(c2_name);
	assert(com2_server_tid >= 0, "Notifier cannot find com2 server's tid");

	char send[2];
	send[1] = '\0';

	while (1) {
		AwaitEvent(EVENT_COM2_RX , send, sizeof(char));
		Send(com2_server_tid, send, 2, NULL, 0);
	}
}

int Getc(int channel) {
	int ret = -1;
	GetcQuery getc_query;
	getc_query.type = IO_QUERY_TYPE_GETC;
	char reply[2];
	if (channel == 1) {
		char c_name[] = COM1_REG_NAME;
		int com1_server_tid = WhoIs(c_name);
		assert(com1_server_tid > 0, "Getc failed to get com1server's tid");
		ret = Send(com1_server_tid, (char *)&getc_query, sizeof(GetcQuery), reply, 2);
	} else {
		char c_name[] = COM2_REG_NAME;
		int com2_server_tid = WhoIs(c_name);
		assert(com2_server_tid > 0, "Getc failed to get com1server's tid");
		ret = Send(com2_server_tid, (char *)&getc_query, sizeof(GetcQuery), reply, 2);
	}
	if (ret < 0) return ret;
	return (int)(reply[0]);
}

int Putc(int channel, char ch) {
	int ret = -1;
	PutcQuery putc_query;
	putc_query.type = IO_QUERY_TYPE_PUTC;
	putc_query.ch = ch;
	if (channel == 1) {
		char c_name[] = COM1_REG_NAME;
		int com1_server_tid = WhoIs(c_name);
		assert(com1_server_tid > 0, "Putc failed to get com1server's tid");
		ret = Send(com1_server_tid, (char *)&putc_query, sizeof(PutcQuery), NULL, 0);
	} else {
		char c_name[] = COM2_REG_NAME;
		int com2_server_tid = WhoIs(c_name);
		assert(com2_server_tid > 0, "Putc failed to get com1server's tid");
		ret = Send(com2_server_tid, (char *)&putc_query, sizeof(PutcQuery), NULL, 0);
	}
	return ret;
}

void comserver() {
	int tid;
	int server_type = -1;
	Receive(&tid, (char *)(&server_type), sizeof(int));
	Reply(tid, NULL, 0);

	char c_name[COM_NAME_ARRAY_LEN] = "";
	if(server_type == COM1) strncpy(c_name, COM1_REG_NAME, COM_NAME_ARRAY_LEN);
	else if(server_type == COM2) strncpy(c_name, COM2_REG_NAME, COM_NAME_ARRAY_LEN);

	assert(strlen(c_name) != 0, "Invalid COM server name");
	assert(RegisterAs(c_name) == 0, "COM server register failed");

	CharBuffer send_buffer;
	CharBuffer receive_buffer;

	int send_notifier_tid;
	int receive_notifier_tid;
	char send_array[COM_BUFFER_SIZE];
	char receive_array[COM_BUFFER_SIZE];

	cBufferInitial(&send_buffer, send_array, COM_BUFFER_SIZE);
	cBufferInitial(&receive_buffer, receive_array, COM_BUFFER_SIZE);
	if (server_type == COM1) {
		send_notifier_tid = Create(1, com1SendNotifier);
		receive_notifier_tid = Create(1, com1ReceiveNotifier);
	} else {
		send_notifier_tid = Create(1, com2SendNotifier);
		receive_notifier_tid = Create(1, com2ReceiveNotifier);
	}

	IOMsg message;
	char reply[2];
	reply[1] = '\0';

	int char_getter_is_waiting = 0;
	int send_notifier_is_waiting = 0;
	int char_getter_tid = 0;

	while(1) {
		/* Receive and process a message */
		Receive(&tid, (char *)(&message), sizeof(IOMsg));

		// If is from send notifier, set send_notifier_is_waiting
		if (tid == send_notifier_tid) {
			send_notifier_is_waiting = 1;
		}
		// Or is from receive notifier,
		else if (tid == receive_notifier_tid) {
			// Reply FIRST, then push the char to receive_buffer
			Reply(tid, reply, 1);
			cBufferPush(&receive_buffer, message.ch);
		}
		// Or is a getc msg, set char_getter_is_waiting and save its tid
		else if (message.type == IO_QUERY_TYPE_GETC) {
			char_getter_is_waiting = 1;
			char_getter_tid = tid;
		}
		// Or is a putc msg,
		else if (message.type == IO_QUERY_TYPE_PUTC) {
			// Push the char to send_buffer, then reply
			cBufferPush(&send_buffer, message.putcQuery.ch);
			Reply(tid, reply, 1);
		}

		/* Serve waiting getter/send notifier */
		if (send_notifier_is_waiting && send_buffer.current_size > 0) {
			reply[0] = cBufferPop(&send_buffer);
			send_notifier_is_waiting = 0;
			Reply(send_notifier_tid, reply, 2);
		}
		if (char_getter_is_waiting && receive_buffer.current_size > 0) {
			reply[0] = cBufferPop(&receive_buffer);
			char_getter_is_waiting = 0;
			Reply(char_getter_tid, reply, 2);
		}
	}
}
