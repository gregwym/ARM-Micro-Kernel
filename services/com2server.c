#include <services.h>
#include <klib.h>
#include <unistd.h>

#define C2_BUFFER_SIZE		100

void static putcToCOM2(char c) {
	int *flags, *data;
	flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
	if (!( *flags & TXFF_MASK )) {
		data = (int *)( UART2_BASE + UART_DATA_OFFSET );
		*data = c;
	} 
	// else {
		// assert(0, "wtf?");
	// }
}

char static getcFromCOM2() {
	int *flags, *ret;
	flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
	if (*flags & RXFF_MASK) {
		ret = (int *)( UART2_BASE + UART_DATA_OFFSET );
		return *ret
	}
	// assert(0, "wtf?");
}

void com2SendNotifier() {
	char c2_name[] = COM2_REG_NAME
	int com2_server_tid = WhoIs(c2_name);
	assert(com2_server_tid >= 0, "Notifier cannot find com2 server's tid");
	
	char send[1] = "";
	while (1) {
		AwaitEvent(/* , , */);
		Send(com2_server_tid, send, 1, NULL, 0);
	}
}

void com2ReceiveNotifier() {
	char c2_name[] = COM2_REG_NAME
	int com2_server_tid = WhoIs(c2_name);
	assert(com2_server_tid >= 0, "Notifier cannot find com2 server's tid");
	
	char send[1] = "";
	while (1) {
		AwaitEvent(/* , , */);
		Send(com2_server_tid, send, 1, NULL, 0);
	}
}

void com2server() {
	char c2_name[] = COM2_REG_NAME;
	assert(RegisterAs(c2_name) == 0, "Clockserver register failed");
	
	char buffer[C2_BUFFER_SIZE];
	CharBuffer send_buffer;
	cBufferInitial(&send_buffer, buffer, c2_BUFFER_SIZE);
	
	int send_notifier_tid = Create(1, com2SendNotifier);
	int receive_notifier_tid = Create(1, com2ReceiveNotifier);
	char ts_name[] = TS_REG_NAME;
	int train_server_tid = WhoIs(ts_name);
	
	int tid;
	char message[2];
	char reply[1] = "";
	char send[2];
	send[1] = '\0';
	
	while(1) {
		Receive(&tid, message, 2);
		switch(tid) {
			case send_notifier_tid:
				Reply(tid, reply, 2);
				putcToCOM2(cBufferPop(&send_buffer));
				if (send_buffer.current_size == 0) {
					//todo: turn off COM1 receive interrupt
				}
				break;
			case receive_notifier_tid:
				send[0] = getcFromCOM2();
				Reply(tid, reply, 2);
				Send(train_server_tid, send, 3, NULL, 0);
				break;
			case train_server_tid;
				Reply(tid, reply, 2);
				cBufferPush(&send_buffer, message[0]);
				if (send_buffer.current_size == 1) {
					//todo: turn on COM1 receive interrupt
				}
				break;
			default:
				assert(0, "wocao?");
				break;
		}
	}
}
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			