#include <klib.h>


static int getParent(int index) {
	return (index-1)/2;
}

static int getLeftChild(int index) {
	return (index+1) * 2 - 1;
}

static int getRightChild(int index) {
	return (index+1) * 2;
}

static void swap(HeapNode **a, HeapNode **b) {
	HeapNode *tmp = *a;
	*a = *b;
	*b = tmp;
}

void heapInitial(Heap *heap, HeapNode **data, int heap_max) {
	heap->maxsize = heap_max;
	heap->heapsize= 0;
	heap->data = data;
	int i = 0;
	for (i = 0; i < heap_max; i++) {
		data[i] = NULL;
	}
}

void heapNodesInitial(HeapNode *nodes, int nodes_num) {
	int i = -1;
	for (i = 0; i < nodes_num; i++) {
		nodes[i].key = -1;
		nodes[i].datum = NULL;
	}
}

void minHeapInsert(Heap *heap, HeapNode *node) {
	assert(heap->heapsize < heap->maxsize, "heapsize exceed");
	int index = heap->heapsize;
	int parent_index = getParent(index);
	HeapNode **data = heap->data;
	data[index] = node;
	while(index > 0 && data[parent_index]->key > data[index]->key) {
		swap(&(data[parent_index]), &(data[index]));
		index = parent_index;
		parent_index = getParent(index);
	}
	heap->heapsize++;
}

HeapNode *minHeapPop(Heap *heap) {
	if (heap->heapsize == 0) {
		return NULL;
	}
	HeapNode *ret = heap->data[0];
	heap->data[0] = heap->data[heap->heapsize - 1];
	heap->data[heap->heapsize - 1] = NULL;
	heap->heapsize--;
	HeapNode **data = heap->data;
	int index = 0;
	int left = getLeftChild(index);
	int right = getRightChild(index);
	while(left <= heap->heapsize - 1) {
		if (right <= heap->heapsize - 1) {
			int tmp_index = data[left]->key < data[right]->key ? left : right;
			if (data[index]->key > data[tmp_index]->key) {
				swap(&(data[index]), &(data[tmp_index]));
				index = tmp_index;
				left = getLeftChild(index);
				right = getRightChild(index);
			} else {
				break;
			}
		} else {
			if (data[index]->key > data[left]->key) {
				swap(&(data[index]), &(data[left]));
				index = left;
				left = getLeftChild(index);
				right = getRightChild(index);
			} else {
				break;
			}
		}
	}
	return ret;
}
