/*
 * Kernel Library
 */
#ifndef __KLIB_H__
#define __KLIB_H__

#include <kern/types.h>

/* String functions */
void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *ptr, int ch, size_t len);
int strcmp(const char *src, const char *dst);
char *strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s);

/* Heap */
typedef struct heapnode {
	int key;
	void *datum;
} HeapNode;

typedef struct heap {
	int maxsize;
	int heapsize;
	HeapNode **data;
} Heap;

void heapInitial(Heap *heap, HeapNode **data, int heap_max);

void heapNodesInitial(HeapNode *nodes, int nodes_num);

// insert to a min-heap
void minHeapInsert(Heap *heap, HeapNode *node);

// pop from a min-heap
HeapNode *minHeapPop(Heap *heap);

/* Char Buffer */
typedef struct charbuffer {
	char *buffer;
	int front;
	int back;
	int current_size;
	int buffer_size;
} CharBuffer;

void cBufferInitial(CharBuffer *char_buffer, char *buffer, int buffer_size);

void cBufferPush(CharBuffer *char_buffer, char c);

char cBufferPop(CharBuffer *char_buffer);

/* Int Buffer */
typedef struct intbuffer {
	int *buffer;
	int front;
	int back;
	int current_size;
	int buffer_size;
} IntBuffer;

void iBufferInitial(IntBuffer *int_buffer, int *buffer, int buffer_size);

void iBufferPush(IntBuffer *int_buffer, int i);

int iBufferPop(IntBuffer *int_buffer);

/* Print */
#include <intern/irqio.h>
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
