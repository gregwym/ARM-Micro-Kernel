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
	BlockedList	*receive_blocked_lists;
	BlockedList	*event_blocked_lists;
	MsgBuffer	*msg_array;
	Task	 	*task_array;
} KernelGlobal;

/* String functions */
void *memcpy(void *dst, const void *src, size_t len);
int strcmp(const char *src, const char *dst);
char *strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s);

/* Heap */
typedef struct heapnode {
	int key;
	void *datum;
} HeapNode;

typedef struct minheap {
	int maxsize;
	int heapsize;
	HeapNode **data;
} MinHeap;

void MinHeapInitial(MinHeap *heap, HeapNode **data, int heap_max);

void HeapNodesInitial(HeapNode *nodes, int nodes_num);

void heapInsert(MinHeap *heap, HeapNode *node);

HeapNode *popMin(MinHeap *heap);

/* Print */
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
