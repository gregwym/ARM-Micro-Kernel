#include <services.h>
#include <klib.h>
#include <unistd.h>

#define cmd_buffer_size 100

void trainserver() {
	// char tr_name[] = TR_REG_NAME;
	// assert(RegisterAs(cs_name) == 0, "Clockserver register failed");
	
	CircularBuffer cmd_buffer;
	char cmd_buffer[cmd_buffer_size];
	bufferInitial(&cmd_buffer, CHARS, cmd_buffer, cmd_buffer_size);
	
	while (1) {
		int ch = Getc(COM2);
		if (ch >= 0) {
			switch((char)ch) {
				case '\b':
					bufferPop(&cmd_buffer);
					break;
				case '\r':
					send_cmd(&cmd_buffer);
					break;
				default:
					bufferPush(&cmd_buffer, ch);
		}
	}
}
			
		
	
	