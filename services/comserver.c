#include <services.h>
#include <klib.h>
#include <unistd.h>

#define COM_BUFFER_SIZE		100
#define IO_QUERY_TYPE_GETC	0
#define IO_QUERY_TYPE_PUTC	1

#define COM1_REG_NAME 	"C1"
#define COM2_REG_NAME 	"C2"

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
	
	char c_name[3] = "";
	if(server_type == COM1) strncpy(c_name, COM1_REG_NAME, 2);
	else if(server_type == COM2) strncpy(c_name, COM2_REG_NAME, 2);
	
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
	
	int ready_to_send_to_char_getter = 0;
	int ready_to_send_to_com = 0;
	int char_getter_tid = 0;
	
	while(1) {
		Receive(&tid, (char *)&message, sizeof(IOMsg));
		if (tid == send_notifier_tid) {
			ready_to_send_to_com = 1;
		} else if (tid == receive_notifier_tid) {
			cBufferPush(&receive_buffer, message.ch);
			Reply(tid, reply, 1);
		} else {
			if (message.type == IO_QUERY_TYPE_GETC) {
				ready_to_send_to_char_getter = 1;
				char_getter_tid = tid;
			} else if (message.type == IO_QUERY_TYPE_PUTC) {
				cBufferPush(&send_buffer, message.putcQuery.ch);
				Reply(tid, reply, 1);
			}
		}
		if (ready_to_send_to_com && send_buffer.current_size > 0) {
			reply[0] = cBufferPop(&send_buffer);
			ready_to_send_to_com = 0;
			Reply(send_notifier_tid, reply, 2);
		}
		if (ready_to_send_to_char_getter && receive_buffer.current_size > 0) {
			reply[0] = cBufferPop(&receive_buffer);
			ready_to_send_to_char_getter = 0;
			Reply(char_getter_tid, reply, 2);
		}
	}
}
