#include <kern/ts7200.h>
#include <klib.h>
#include <unistd.h>

typedef struct node{
	int value;
} testnode;

void test() {
	int i;
	testnode tnode[10];
	for (i = 0; i < 10; i++) {
		tnode[i].value = i * 2;
	}
	Heap minheap;
	HeapNode hnode[10];
	HeapNode *heap_data[10];
	heapNodesInitial(hnode, 10);
	heapInitial(&minheap, heap_data, 10);
	for (i = 0; i < 10; i++) {
		hnode[i].datum = &tnode[i];
		hnode[i].key = i;
	}
	

	minHeapInsert(&minheap, &hnode[2]);
	// bwprintf("cnm %d\n", ((testnode *)(minheap.data[0]->datum))->value);
	minHeapInsert(&minheap, &hnode[5]);
	minHeapInsert(&minheap, &hnode[3]);
	minHeapInsert(&minheap, &hnode[8]);
	minHeapInsert(&minheap, &hnode[9]);
	minHeapInsert(&minheap, &hnode[0]);
	
	HeapNode *tmp = minHeapPop(&minheap);
	while (tmp != NULL) {
		bwprintf(COM2, "heap head: %d\n", ((testnode *)(tmp->datum))->value);
		tmp = minHeapPop(&minheap);
	}
}

void umain() {
	int tid;
	tid = Create(1, test);
	bwprintf(COM2, "Created: %d\n", tid);
}