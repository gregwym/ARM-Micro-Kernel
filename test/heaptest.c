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
	MinHeap minheap;
	HeapNode hnode[10];
	HeapNode *heap_data[10];
	HeapNodesInitial(hnode, 10);
	MinHeapInitial(&minheap, heap_data, 10);
	for (i = 0; i < 10; i++) {
		hnode[i].datum = &tnode[i];
		hnode[i].key = i;
	}
	

	heapInsert(&minheap, &hnode[2]);
	// bwprintf("cnm %d\n", ((testnode *)(minheap.data[0]->datum))->value);
	heapInsert(&minheap, &hnode[5]);
	heapInsert(&minheap, &hnode[3]);
	heapInsert(&minheap, &hnode[8]);
	heapInsert(&minheap, &hnode[9]);
	heapInsert(&minheap, &hnode[0]);
	
	HeapNode *tmp = popMin(&minheap);
	while (tmp != NULL) {
		bwprintf(COM2, "heap head: %d\n", ((testnode *)(tmp->datum))->value);
		tmp = popMin(&minheap);
	}
}

void umain() {
	int tid;
	tid = Create(1, test);
	bwprintf(COM2, "Created: %d\n", tid);
}