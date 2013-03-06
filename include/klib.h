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
char *strncat(char *dst, const char *src, size_t n);
size_t strlen(const char *s);

/* Heap */
typedef struct heapnode {
	int key;
	int index;
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

void minHeapResortNode(Heap *heap, HeapNode *node);

/* Circular Buffer */
typedef enum buffer_type {
	CHARS,
	INTS,
	POINTERS,
	VOIDS,
} BufferType;

typedef union buffer_array {
	char *chars;
	int  *ints;
	void **pointers;
	void *voids;
} BufferArray;

typedef struct circular_buffer {
	BufferType type;
	int front;
	int back;
	int current_size;
	int buffer_size;
	BufferArray buffer;
} CircularBuffer;

void bufferInitial(CircularBuffer *circ_buffer, BufferType type, void *buffer, int buffer_size);

void bufferPush(CircularBuffer *circ_buffer, int i);

int bufferPop(CircularBuffer *circ_buffer);

void bufferPushStr(CircularBuffer *circ_buffer, char *str, int strlen);

void bufferPushChar(CircularBuffer *circ_buffer, char c);

/* Print */
#include <intern/irqio.h>
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
