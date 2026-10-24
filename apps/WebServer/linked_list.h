#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

typedef struct node_struct {
	void *data;
	struct node_struct *next;
	struct node_struct *prev;
	void (*destroy)(void *);
} *node;

typedef struct linked_list_struct {
	node head;
	node tail;
} *linked_list;

extern linked_list new_linked_list(void);
extern node new_node(void *data, void (*destroy)(void *));
extern void insert_tail(linked_list list, void *data, void (*destroy)(void *));
extern void *remove_tail(linked_list list);
extern void free_linked_list(linked_list list);

#endif /* _LINKED_LIST_H_ */
