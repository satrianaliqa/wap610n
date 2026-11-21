#include <stdlib.h>
#include <string.h>
#include <limits.h>
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
	xml_element el;

	if (name == NULL) {
		return NULL;
	}
	el = (xml_element)malloc(sizeof(struct xml_element_struct));
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

	if (el == NULL) {
		return;
	}
	if (el->name != NULL) {
		free(el->name);
	}
	if (el->text != NULL) {
		free(el->text);
	}
	if (el->attributes != NULL) {
		free_linked_list(el->attributes);
	}
	free(el);
}

void add_xml_element_attr(xml_element el, attribute attr)
{
	if (el == NULL || attr == NULL || el->attributes == NULL) {
		return;
	}
	insert_tail(el->attributes, attr, free_attribute);
}

void append_xml_element_text(xml_element el, const char *text, int len)
{
	int old_len;
	char *new_text;

	if (el == NULL || text == NULL || len <= 0) {
		return;
	}
	if (el->text != NULL) {
		old_len = (int)strlen(el->text);
		if (len > INT_MAX - old_len - 1) {
			return;
		}
		new_text = (char *)realloc(el->text, (size_t)old_len + (size_t)len + 1);
		if (new_text == NULL) {
			return;
		}
		el->text = new_text;
		memcpy(new_text + old_len, text, (size_t)len);
		el->text[old_len + len] = '\0';
	} else {
		if (len > INT_MAX - 1) {
			return;
		}
		new_text = (char *)malloc((size_t)len + 1);
		if (new_text == NULL) {
			return;
		}
		el->text = new_text;
		memcpy(new_text, text, (size_t)len);
		el->text[len] = '\0';
	}
}

void set_xml_element_text(xml_element el, const char *text)
{
	if (el == NULL || text == NULL) {
		return;
	}
	if (*text == '\0') {
		if (el->text != NULL) {
			free(el->text);
		}
		el->text = (char *)malloc(1);
		if (el->text != NULL) {
			el->text[0] = '\0';
		}
		return;
	}
	append_xml_element_text(el, text, (int)strlen(text));
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
	xml_element next;

	while (el != NULL) {
		next = el->next_sibling;
		if (el->first_child != NULL) {
			deep_free_xml_element(el->first_child);
		}
		free_xml_element(el);
		el = next;
	}
}

void push_xml_tree(xml_tree tree, xml_element el)
{
	xml_element parent, curr;

	if (tree == NULL || el == NULL || tree->element_stack == NULL) {
		return;
	}
	if (tree->root == NULL) {
		tree->root = el;
	} else {
		parent = (xml_element)peek_stack(tree->element_stack);
		if (parent == NULL) {
			return;
		}
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
	if (tree == NULL || tree->element_stack == NULL) {
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

	if (tree == NULL || s == NULL || len <= 0) {
		return;
	}
	el = (xml_element)peek_stack(tree->element_stack);
	if (el != NULL) {
		append_xml_element_text(el, s, len);
	}
}

static char *append_xml_str(char *p, const char *end, const char *s)
{
	size_t slen;
	size_t avail;

	if (p == NULL || s == NULL) {
		return p;
	}
	slen = strlen(s);
	if (end != NULL) {
		if (p >= end) {
			return p;
		}
		avail = (size_t)(end - p);
		if (slen >= avail) {
			if (avail > 1) {
				memcpy(p, s, avail - 1);
				p += (avail - 1);
			}
			*p = '\0';
			return p;
		}
	}
	memcpy(p, s, slen);
	p += slen;
	*p = '\0';
	return p;
}

static char *generate_xml_cursor(xml_element el, char *p, const char *end)
{
	node curr_node;
	attribute attr;

	while (el != NULL && p != NULL) {
		if (el->name == NULL) {
			el = el->next_sibling;
			continue;
		}
		p = append_xml_str(p, end, "<");
		p = append_xml_str(p, end, el->name);
		if (el->attributes != NULL) {
			curr_node = el->attributes->head;
			while (curr_node != NULL) {
				attr = (attribute)curr_node->data;
				if (attr != NULL && attr->name != NULL && attr->value != NULL) {
					p = append_xml_str(p, end, " ");
					p = append_xml_str(p, end, attr->name);
					p = append_xml_str(p, end, "=");
					p = append_xml_str(p, end, attr->value);
				}
				curr_node = curr_node->next;
			}
		}
		p = append_xml_str(p, end, ">");
		if (el->text != NULL) {
			p = append_xml_str(p, end, el->text);
		}
		if (el->first_child != NULL) {
			p = append_xml_str(p, end, "\r\n");
			p = generate_xml_cursor(el->first_child, p, end);
		}
		p = append_xml_str(p, end, "</");
		p = append_xml_str(p, end, el->name);
		p = append_xml_str(p, end, ">\r\n");
		el = el->next_sibling;
	}
	return p;
}

void generate_xml_helper(xml_element el, char *buf)
{
	if (el == NULL || buf == NULL) {
		return;
	}
	generate_xml_cursor(el, buf + strlen(buf), NULL);
}

void generate_xml(xml_tree tree, char *buf)
{
	if (tree == NULL || buf == NULL || tree->root == NULL) {
		return;
	}
	generate_xml_cursor(tree->root, buf + strlen(buf), NULL);
}

void generate_xml_n(xml_tree tree, char *buf, size_t buf_size)
{
	size_t cur_len;

	if (tree == NULL || buf == NULL || buf_size == 0 || tree->root == NULL) {
		return;
	}
	cur_len = strlen(buf);
	if (cur_len >= buf_size - 1) {
		return;
	}
	generate_xml_cursor(tree->root, buf + cur_len, buf + buf_size);
}
