#include "type.h"
#include "syscall_no.h"
//#include "ourlib.h"


void syscall(int *tf) {
	int *ptr = 0x28;
	
	switch (tf[0]) {
		case SYS_exit:
			//todo
			break;
		case SYS_create:
			*ptr = 0x218000 + tlistPush;
			asm("swi 0");
			
			break;
		case SYS_pass:
			//todo
			break;
		default:
			assert(1, "Invalid syscall number");
			break;
	}
	
	//get return value
	
	//add pc
	
}