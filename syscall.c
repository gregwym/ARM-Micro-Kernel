#include <stdlib.h>
#include <syscall_handler.h>
#include <bwio.h>
#include <usertrap.h>

int Create(int priority, void (*code) ()) {
	asm("swi 1");
}

int MyTid() {

}

int MyParentTid() {

}

void Pass() {

}

void Exit() {
	asm("swi 0");
}
