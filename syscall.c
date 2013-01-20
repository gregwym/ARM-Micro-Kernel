<<<<<<< HEAD:lib/syscall.c
#include "type.h"
#include "syscall_no.h"
//#include "ourlib.h"
=======
#include <type.h>
#include <syscall_no.h>
#include <stdlib.h>
>>>>>>> 8e664c515ac195f387880710fbee281105600bc5:syscall.c


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