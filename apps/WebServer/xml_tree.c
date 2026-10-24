#include <stdlib.h>
#include <string.h>
#include "xml_tree.h"

attribute new_attribute(const char *name, const char *value)
{
	attribute attr;
	if (name == NULL || value == NULL) {
		return NULL;
	}
	attr = (attribute)malloc(sizeof(struct attribute_struct));
	if (attr == NULL) {
		return NULL;
	}
	attr->name = (char *)malloc(strlen(name) + 1);
	if (attr->name == NULL) {
		free(attr);
		return NULL;
	}
	strcpy(attr->name, name);
	attr->value = (char *)malloc(strlen(value) + 1);
	if (attr->value == NULL) {
		free(attr->name);
		free(attr);
		return NULL;
	}
	strcpy(attr->value, value);
	return attr;
}

void free_attribute(void *p)
{
	attribute attr = (attribute)p;
	if (attr == NULL) {
		return;
	}
	if (attr->name != NULL) {
		free(attr->name);
	}
	if (attr->value != NULL) {
		free(attr->value);
	}
	free(attr);
}

xml_element new_xml_element(const char *name)
{
	xml_element el = (xml_element)malloc(sizeof(struct xml_element_struct));
	if (el == NULL) {
		return NULL;
	}
	el->name = (char *)malloc(strlen(name) + 1);
	if (el->name == NULL) {
		free(el);
		return NULL;
	}
	strcpy(el->name, name);
	el->attributes = new_linked_list();
	if (el->attributes == NULL) {
		free(el->name);
		free(el);
		return NULL;
	}
	el->next_sibling = NULL;
	el->first_child = NULL;
	el->text = NULL;
	return el;
}

void free_xml_element(void *p)
{
	xml_element el = (xml_element)p;
	if (el->name != NULL) {
		free(el->name);
	}
	if (el->text != NULL) {
		free(el->text);
	}
	free_linked_list(el->attributes);
	free(el);
}

void add_xml_element_attr(xml_element el, attribute attr)
{
	if (el == NULL || attr == NULL) {
		return;
	}
	insert_tail(el->attributes, attr, free_attribute);
}

void append_xml_element_text(xml_element el, const char *text, int len)
{
	int old_len;
	char *new_text;
	if (el == NULL || text == NULL) {
		return;
	}
	if (el->text != NULL) {
		old_len = strlen(el->text);
		new_text = (char *)realloc(el->text, old_len + len + 1);
		el->text = new_text;
		if (new_text == NULL) {
			return;
		}
		strncat(new_text, text, len);
		el->text[old_len + len] = '\0';
	} else {
		new_text = (char *)malloc(len + 1);
		el->text = new_text;
		if (new_text == NULL) {
			return;
		}
		memcpy(new_text, text, len);
		el->text[len] = '\0';
	}
}

void set_xml_element_text(xml_element el, const char *text)
{
	append_xml_element_text(el, text, strlen(text));
}

xml_tree new_xml_tree(void)
{
	xml_tree tree = (xml_tree)malloc(sizeof(struct xml_tree_struct));
	if (tree == NULL) {
		return NULL;
	}
	tree->root = NULL;
	tree->element_stack = new_stack();
	if (tree->element_stack == NULL) {
		free(tree);
		return NULL;
	}
	return tree;
}

void deep_free_xml_element(xml_element el)
{
	if (el->first_child != NULL) {
		deep_free_xml_element(el->first_child);
	}
	if (el->next_sibling != NULL) {
		deep_free_xml_element(el->next_sibling);
	}
	free_xml_element(el);
}

void push_xml_tree(xml_tree tree, xml_element el)
{
	xml_element parent, curr;
	if (tree == NULL || el == NULL) {
		return;
	}
	if (tree->root == NULL) {
		tree->root = el;
	} else {
		parent = (xml_element)peek_stack(tree->element_stack);
		if (parent->first_child == NULL) {
			parent->first_child = el;
		} else {
			curr = parent->first_child;
			while (curr->next_sibling != NULL) {
				curr = curr->next_sibling;
			}
			curr->next_sibling = el;
		}
	}
	push_stack(tree->element_stack, el, free_xml_element);
}

void pop_all_xml_tree(xml_tree tree)
{
	if (tree == NULL) {
		return;
	}
	while (pop_stack(tree->element_stack) != NULL) {
	}
}

void deep_free_xml_tree(xml_tree tree)
{
	if (tree == NULL) {
		return;
	}
	pop_all_xml_tree(tree);
	if (tree->root != NULL) {
		deep_free_xml_element(tree->root);
	}
	free_stack(tree->element_stack);
	free(tree);
}

void push_pop_xml_tree(xml_tree tree, xml_element el)
{
	if (tree == NULL || el == NULL || tree->element_stack == NULL) {
		return;
	}
	push_xml_tree(tree, el);
	pop_stack(tree->element_stack);
}

void pop_xml_tree(xml_tree tree)
{
	if (tree == NULL || tree->element_stack == NULL) {
		return;
	}
	pop_stack(tree->element_stack);
}

void add_text_xml_tree(xml_tree tree, const char *s, int len)
{
	xml_element el;
	if (tree == NULL || s == NULL) {
		return;
	}
	el = (xml_element)peek_stack(tree->element_stack);
	if (el != NULL) {
		append_xml_element_text(el, s, len);
	}
}

void generate_xml_helper(xml_element el, char *buf)
{
	node curr_node;
	attribute attr;
	while (el != NULL && buf != NULL) {
		strcat(buf, "<");
		strcat(buf, el->name);
		curr_node = el->attributes->head;
		while (curr_node != NULL) {
			attr = (attribute)curr_node->data;
			strcat(buf, " ");
			strcat(buf, attr->name);
			strcat(buf, "=");
			strcat(buf, attr->value);
			curr_node = curr_node->next;
		}
		strcat(buf, ">");
		if (el->text != NULL) {
			strcat(buf, el->text);
		}
		if (el->first_child != NULL) {
			strcat(buf, "\r\n");
			generate_xml_helper(el->first_child, buf);
		}
		strcat(buf, "</");
		strcat(buf, el->name);
		strcat(buf, ">");
		strcat(buf, "\r\n");
		el = el->next_sibling;
	}
}

void generate_xml(xml_tree tree, char *buf)
{
	if (tree == NULL || buf == NULL) {
		return;
	}
	generate_xml_helper(tree->root, buf);
}
