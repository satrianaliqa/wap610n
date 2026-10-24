#include <stdlib.h>
#include "stack.h"

stack new_stack(void)
{
	stack s = (stack)malloc(sizeof(struct stack_struct));
	if (s != NULL) {
		s->list = new_linked_list();
		if (s->list == NULL) {
			free(s);
			return NULL;
		}
	}
	return s;
}

void free_stack(stack s)
{
	if (s == NULL) {
		return;
	}
	free_linked_list(s->list);
	free(s);
}

void push_stack(stack s, void *data, void (*destroy)(void *))
{
	if (s == NULL || data == NULL || destroy == NULL) {
		return;
	}
	insert_tail(s->list, data, destroy);
}

void *pop_stack(stack s)
{
	if (s == NULL) {
		return NULL;
	}
	return remove_tail(s->list);
}

void *peek_stack(stack s)
{
	if (s == NULL || s->list->tail == NULL) {
		return NULL;
	}
	return s->list->tail->data;
}
