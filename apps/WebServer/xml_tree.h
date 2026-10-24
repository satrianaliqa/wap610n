#ifndef _XML_TREE_H_
#define _XML_TREE_H_

#include "linked_list.h"
#include "stack.h"

typedef struct attribute_struct {
	char *name;
	char *value;
} *attribute;

typedef struct xml_element_struct {
	char *name;
	linked_list attributes;
	struct xml_element_struct *next_sibling;
	struct xml_element_struct *first_child;
	char *text;
} *xml_element;

typedef struct xml_tree_struct {
	xml_element root;
	stack element_stack;
} *xml_tree;

extern attribute new_attribute(const char *name, const char *value);
extern void free_attribute(void *attr);
extern xml_element new_xml_element(const char *name);
extern void free_xml_element(void *el);
extern void add_xml_element_attr(xml_element el, attribute attr);
extern void append_xml_element_text(xml_element el, const char *text, int len);
extern void set_xml_element_text(xml_element el, const char *text);
extern xml_tree new_xml_tree(void);
extern void deep_free_xml_element(xml_element el);
extern void push_xml_tree(xml_tree tree, xml_element el);
extern void pop_all_xml_tree(xml_tree tree);
extern void deep_free_xml_tree(xml_tree tree);
extern void push_pop_xml_tree(xml_tree tree, xml_element el);
extern void pop_xml_tree(xml_tree tree);
extern void add_text_xml_tree(xml_tree tree, const char *s, int len);
extern void generate_xml_helper(xml_element el, char *buf);
extern void generate_xml(xml_tree tree, char *buf);

#endif /* _XML_TREE_H_ */
