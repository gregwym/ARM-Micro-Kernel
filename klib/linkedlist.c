#include <klib.h>

void listInitial(LinkedList *list) {
	list->head = NULL;
	list->tail = NULL;
}

void listPush(LinkedList *list, LinkedListNode *node) {
	node->previous = NULL;
	node->next = list->head;
	node->next->previous = node;
	list->head = node;
}

void listAppend(LinkedList *list, LinkedListNode *node) {
	node->previous = list->tail;
	node->previous->next = node;
	node->next = NULL;
	list->tail = node;
}

void listRemove(LinkedList *list, LinkedListNode *node) {
	node->previous->next = node->next;
	node->next->previous = node->previous;
	if (node == list->head) {
		list->head = node->next;
	} else if (node == list->tail) {
		list->tail = node->previous;
	}
	node->next = NULL;
	node->previous = NULL;
}
