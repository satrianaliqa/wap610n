#include <stdlib.h>
#include "linked_list.h"

linked_list new_linked_list(void)
{
	linked_list list = (linked_list)malloc(sizeof(struct linked_list_struct));
	if (list != NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	return list;
}

node new_node(void *data, void (*destroy)(void *))
{
	node n;
	if (data == NULL || destroy == NULL) {
		return NULL;
	}
	n = (node)malloc(sizeof(struct node_struct));
	if (n != NULL) {
		n->data = data;
		n->next = NULL;
		n->prev = NULL;
		n->destroy = destroy;
	}
	return n;
}

void insert_tail(linked_list list, void *data, void (*destroy)(void *))
{
	node n;
	if (list == NULL || data == NULL || destroy == NULL) {
		return;
	}
	n = new_node(data, destroy);
	if (n == NULL) {
		return;
	}
	if (list->head == NULL) {
		list->head = n;
	} else {
		list->tail->next = n;
		n->prev = list->tail;
	}
	list->tail = n;
}

void *remove_tail(linked_list list)
{
	node n;
	void *data;
	if (list == NULL || list->head == NULL || list->tail == NULL) {
		return NULL;
	}
	n = list->tail;
	data = n->data;
	if (list->head == n) {
		list->head = NULL;
		list->tail = NULL;
	} else {
		list->tail = n->prev;
		list->tail->next = NULL;
	}
	free(n);
	return data;
}

void free_linked_list(linked_list list)
{
	node curr, next;
	if (list == NULL) {
		return;
	}
	curr = list->head;
	while (curr != NULL) {
		next = curr->next;
		if (curr->destroy != NULL && curr->data != NULL) {
			curr->destroy(curr->data);
		}
		free(curr);
		curr = next;
	}
	free(list);
}
