/*
 * Kernel Library
 */
#ifndef __KLIB_H__
#define __KLIB_H__

typedef struct kernel_global {
	TaskList 	*task_list;
	FreeList 	*free_list;
	BlockedList	*blocked_list;
	MsgBuffer	*msg_array;
	Task	 	*task_array;
} KernelGlobal;

void assert(int boolean, char * msg);

/* String functions */
int strcmp(const char *src, const char *dst);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, int n);

/* Print */
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
