#include <services.h>
#include <klib.h>
#include <unistd.h>

#define COM_BUFFER_SIZE			1024
#define SEND_NOTIFIER_PRIORITY	0
#define RECEIVE_NOTIFIER_PRIORITY	0

#define IO_QUERY_TYPE_GETC		0
#define IO_QUERY_TYPE_PUTC		1
#define IO_QUERY_TYPE_PUTS		2

#define GETTER_BUFFER_SIZE		10

typedef struct putc_query {
	int type;
	char ch;
} PutcQuery;

typedef struct getc_query {
	int type;
} GetcQuery;

typedef struct puts_query {
	int type;
	char *msg;
	int msglen;
} PutsQuery;

typedef union {
	int 		type;
	char		ch;
	PutcQuery	putcQuery;
	PutsQuery	putsQuery;
	GetcQuery	getcQuery;
} IOMsg;

void comSendNotifier(int channel_id) {
	int server_tid = MyParentTid();
	unsigned int eventId = (channel_id == COM1 ? EVENT_COM1_TX : EVENT_COM2_TX);

	char send_buffer[COM_BUFFER_SIZE];
	int send_size = 0;
	char empty = '\0';
	int result = -1;
	int i;

	while (1) {
		result = Send(server_tid, &empty, sizeof(char), send_buffer, COM_BUFFER_SIZE);
		send_size = result;
		assert(result > 0, "Send Notifier got invalid reply size");
		for (i = 0; i < send_size; i++) {
			result = AwaitEvent(eventId , &(send_buffer[i]), sizeof(char));
			assert(result == 0, "Send Notifier wait event failed");
		}
	}
}

void comReceiveNotifier(int channel_id) {
	int server_tid = MyParentTid();
	unsigned int eventId = (channel_id == COM1 ? EVENT_COM1_RX : EVENT_COM2_RX);

	char ch = '\0';
	int result = -1;

	while (1) {
		result = AwaitEvent(eventId , &ch, sizeof(char));
		assert(result == 0, "Receive Notifier wait event failed");

		result = Send(server_tid, &ch, sizeof(char), NULL, 0);
		assert(result == 0, "Receive Notifier sent to server failed");
	}
}

int Getc(int server_tid) {
	int reply;
	GetcQuery getc_query;
	getc_query.type = IO_QUERY_TYPE_GETC;

	int rtn = Send(server_tid, (char *)(&getc_query), sizeof(GetcQuery), (char *)(&reply), sizeof(int));

	return rtn < 0 ? rtn : reply;
}

int Putc(int server_tid, char ch) {
	PutcQuery putc_query;
	putc_query.type = IO_QUERY_TYPE_PUTC;
	putc_query.ch = ch;

	int rtn = Send(server_tid, (char *)(&putc_query), sizeof(PutcQuery), NULL, 0);

	return rtn;
}

int Puts(int server_tid, char *msg, int msglen) {
	PutsQuery puts_query;
	puts_query.type = IO_QUERY_TYPE_PUTS;
	puts_query.msg = msg;
	if (msglen == 0) {
		puts_query.msglen = strlen(msg);
	} else {
		puts_query.msglen = msglen;
	}

	int rtn = Send(server_tid, (char *)(&puts_query), sizeof(PutsQuery), NULL, 0);
	return rtn;
}

void comserver(int channel_id) {
	int tid;
	int result = RegisterAs(channel_id == COM1 ? COM1_REG_NAME : COM2_REG_NAME);
	assert(result == 0, "COM server register failed");

	// Create send and receive buffer
	char send_array[COM_BUFFER_SIZE];
	int send_size = 0;

	CircularBuffer receive_buffer;
	char receive_array[COM_BUFFER_SIZE];
	bufferInitial(&receive_buffer, CHARS, receive_array, COM_BUFFER_SIZE);

	// Create send and receive notifier
	int send_notifier_tid;
	int receive_notifier_tid;
	send_notifier_tid = CreateWithArgs(SEND_NOTIFIER_PRIORITY, comSendNotifier, channel_id, 0, 0, 0);
	receive_notifier_tid = CreateWithArgs(RECEIVE_NOTIFIER_PRIORITY, comReceiveNotifier, channel_id, 0, 0, 0);

	// Prepare message and reply data structure
	IOMsg message;
	int reply;

	// Create getter buffer
	CircularBuffer getter_buffer;
	int getter_array[GETTER_BUFFER_SIZE];

	bufferInitial(&getter_buffer, INTS, getter_array, GETTER_BUFFER_SIZE);

	// Prepare flag variables
	int send_notifier_is_waiting = 0;

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
			Reply(tid, NULL, 0);
			bufferPushChar(&receive_buffer, message.ch);
			// If is from COM2, echo it
			if(channel_id == COM2) {
				send_array[send_size] = message.ch;
				send_size++;
				if (message.ch == '\b') {
					send_array[send_size] = '\e';
					send_size++;
					send_array[send_size] = '[';
					send_size++;
					send_array[send_size] = 'K';
					send_size++;
				}
			}
		}
		// Or is a getc msg, set char_getter_is_waiting and save its tid
		else if (message.type == IO_QUERY_TYPE_GETC) {
			bufferPush(&getter_buffer, tid);
		}
		// Or is a putc msg,
		else if (message.type == IO_QUERY_TYPE_PUTC) {
			// Push the char to send_buffer, then reply
			send_array[send_size] = message.putcQuery.ch;
			send_size++;
			Reply(tid, NULL, 0);
		}
		// Or is a puts msg,
		else if (message.type == IO_QUERY_TYPE_PUTS) {
			// copy the whole string into send_buffer, then reply
			int i = 0;
			for (i = 0; i < message.putsQuery.msglen; i++) {
				send_array[send_size] = message.putsQuery.msg[i];
				send_size++;
			}
			Reply(tid, NULL, 0);
		}
		/* Serve waiting getter/send notifier */
		if (send_notifier_is_waiting && send_size > 0) {
			send_notifier_is_waiting = 0;
			Reply(send_notifier_tid, send_array, send_size);
			send_size = 0;
		}
		if (getter_buffer.current_size > 0 && receive_buffer.current_size > 0) {
			reply = bufferPop(&receive_buffer);
			Reply(bufferPop(&getter_buffer), (char *)(&reply), sizeof(int));
		}
	}
}
