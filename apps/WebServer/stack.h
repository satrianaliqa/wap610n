#ifndef _STACK_H_
#define _STACK_H_

#include "linked_list.h"

typedef struct stack_struct {
	linked_list list;
} *stack;

extern stack new_stack(void);
extern void free_stack(stack s);
extern void push_stack(stack s, void *data, void (*destroy)(void *));
extern void *pop_stack(stack s);
extern void *peek_stack(stack s);

#endif /* _STACK_H_ */
