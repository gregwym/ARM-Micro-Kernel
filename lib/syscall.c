#include "type.h"
#include "syscall_no.h"
#include "ourlib.h"


void syscall(int *tf) {
	
	switch (tf[?]) {
		case SYS_exit:
			//todo
			break;
		case SYS_create:
			//todo
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