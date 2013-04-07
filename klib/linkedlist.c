#include <klib.h>

void listInitial(LinkedList *list) {
	list->head = NULL;
	list->tail = NULL;
}

void listPush(LinkedList *list, LinkedListNode *node) {
	node->previous = NULL;
	node->next = list->head;
	if(node->next != NULL) {
		node->next->previous = node;
	}
	list->head = node;
	if (list->tail == NULL) {
		list->tail = node;
	}
}

void listAppend(LinkedList *list, LinkedListNode *node) {
	node->previous = list->tail;
	if(node->previous != NULL) {
		node->previous->next = node;
	}
	node->next = NULL;
	list->tail = node;
	if (list->head == NULL) {
		list->head = node;
	}
}

void listRemove(LinkedList *list, LinkedListNode *node) {
	// if (node->next == NULL && node->previous == NULL) {
		// return;
	// }
	LinkedListNode *current_node;
	for (current_node = list->head; current_node != NULL; current_node = current_node->next) {
		if (current_node == node) break;
	}
	if (current_node != node) {
		return;
	}
	
	if(node->previous != NULL) {
		node->previous->next = node->next;
	}
	if(node->next != NULL) {
		node->next->previous = node->previous;
	}
	if (node == list->head) {
		list->head = node->next;
	}
	if (node == list->tail) {
		list->tail = node->previous;
	}
	node->next = NULL;
	node->previous = NULL;
}
