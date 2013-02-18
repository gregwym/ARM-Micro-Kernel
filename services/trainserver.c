#include <services.h>
#include <klib.h>
#include <unistd.h>

void trainserver() {
	char tr_name[] = TR_REG_NAME;
	assert(RegisterAs(cs_name) == 0, "Clockserver register failed");
	
	while (1) {
		int ch = Getc(2);
		if (ch >= 0) {
			Putc(2, (char)ch);
		}
	}
}
			
		
	
	