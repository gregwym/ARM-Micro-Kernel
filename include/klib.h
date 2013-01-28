/*
 * Kernel Library
 */
#ifndef __KLIB_H__
#define __KLIB_H__

#include <task.h>
#include <kern/types.h>

typedef struct kernel_global {
	TaskList 	*task_list;
	FreeList 	*free_list;
	BlockedList	*blocked_list;
	MsgBuffer	*msg_array;
	Task	 	*task_array;
} KernelGlobal;

/* String functions */
void *memcpy(void *dst, const void *src, size_t len);
int strcmp(const char *src, const char *dst);
char *strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s);

/* Print */
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
