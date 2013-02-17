#include <services.h>
#include <klib.h>
#include <unistd.h>

#define C1_BUFFER_SIZE		100

void static putcToCOM1(char c) {
	int *flags, *data;
	flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
	if (!( *flags & TXFF_MASK )) {
		data = (int *)( UART1_BASE + UART_DATA_OFFSET );
		*data = c;
	} 
	// else {
		// assert(0, "wtf?");
	// }
}

char static getcFromCOM1() {
	int *flags, *ret;
	flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
	if (*flags & RXFF_MASK) {
		ret = (int *)( UART1_BASE + UART_DATA_OFFSET );
		return *ret
	}
	// assert(0, "wtf?");
}

void com1SendNotifier() {
	char c1_name[] = COM1_REG_NAME
	int com1_server_tid = WhoIs(c1_name);
	assert(com1_server_tid >= 0, "Notifier cannot find com1 server's tid");
	
	char send[1] = "";
	while (1) {
		AwaitEvent(/* , , */);
		Send(com1_server_tid, send, 1, NULL, 0);
	}
}

void com1ReceiveNotifier() {
	char c1_name[] = COM1_REG_NAME
	int com1_server_tid = WhoIs(c1_name);
	assert(com1_server_tid >= 0, "Notifier cannot find com1 server's tid");
	
	char send[1] = "";
	while (1) {
		AwaitEvent(/* , , */);
		Send(com1_server_tid, send, 1, NULL, 0);
	}
}

void com1server() {
	char c1_name[] = COM1_REG_NAME;
	assert(RegisterAs(c1_name) == 0, "Clockserver register failed");
	
	char buffer[C1_BUFFER_SIZE];
	CharBuffer send_buffer;
	cBufferInitial(&send_buffer, buffer, C1_BUFFER_SIZE);
	
	int send_notifier_tid = Create(1, com1SendNotifier);
	int receive_notifier_tid = Create(1, com1ReceiveNotifier);
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
				putcToCOM1(cBufferPop(&send_buffer));
				if (send_buffer.current_size == 0) {
					//todo: turn off COM1 receive interrupt
				}
				break;
			case receive_notifier_tid:
				send[0] = getcFromCOM1();
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
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			