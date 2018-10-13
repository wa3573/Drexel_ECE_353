/*
 * list_array.cpp
 *
 *
 *	Stack: pointer implementation
 *
 *  Created on: Jul 11, 2018
 *      Author: Juniper
 */

#include "list.h"

list* list_init(list* L, size_t element_size, free_function free_fn, compare_function compare_fn)
{
	L = (list*)malloc(sizeof(list));

	if (L == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	L->head = (node*)malloc(sizeof(node));
	L->tail = (node*)malloc(sizeof(node));

	if (L->head == NULL || L->tail == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	L->free_fn = free_fn;
	L->compare_fn = compare_fn;
	L->logical_length = 0;
	L->element_size = element_size;

	L->head->next = L->tail;
	L->head->prev = NULL;
	L->head->data = NULL;
	L->tail->next = NULL;
	L->tail->prev = L->head;
	L->tail->data = NULL;

	return L;
}

node* list_front(list* L)
{
	return (node*)L->head->next;
}

node* list_back(list* L)
{
	return (node*)L->tail->prev;
}

void list_push_front(list* L, void* data)
{
	node* new = (node*)malloc(sizeof(node));
	new->data = malloc(L->element_size);
	new->prev = L->head;

	memcpy(new->data, data, L->element_size);

	node* p = L->head;
	node* temp = L->head->next;

	p->next = new;
	new->prev = p;
	new->next = temp;
	temp->prev = new;

	L->logical_length++;
}

void list_push_back(list* L, void* data)
{
	node* new = (node*)malloc(sizeof(node));
	new->data = malloc(L->element_size);
	new->next = L->tail;

	memcpy(new->data, data, L->element_size);

	node* p = L->tail->prev;

	p->next = new;
	new->next = L->tail;
	new->prev = p;
	L->tail->prev = new;

	L->logical_length++;
}

void list_pop_front(list* L)
{
	if (L->head->next == L->tail)
	{
		fprintf(stderr, "Error: list is empty \n");
	} else {
		node* temp = L->head->next;
		node* p = temp->next;

		L->head->next = p;
		p->prev = L->head;
		free(temp);

		L->logical_length--;
	}
}

void list_remove_node(list* L, node* p)
{
	if (p == L->head)
	{
		fprintf(stderr, "Error: cannot delete head \n");
	} else if (p == L->tail) {
		fprintf(stderr, "Error: cannot delete head \n");
	} else {
		node* temp = p;
		node* q = p->prev;
		node* r = p->next;

		q->next = r;
		r->prev = q;
		free(temp);

		L->logical_length--;
	}
}

void list_remove_pid(list* L, void* data)
{
	node* p = list_front(L);
	pid_t* ldata = (pid_t*)data;

	while (p->next != NULL)
	{
		if (*((pid_t*)(p->data)) == *ldata)
		{
			list_remove_node(L, p);
			break;
		}

		p = p->next;
	}
}

void list_pop_back(list* L)
{
	if (L->tail->prev == L->head)
	{

	} else {
		node* temp = L->tail->prev;
		node* p = temp->prev;

		L->tail->prev = p;
		p->next = L->tail;

		free(temp);

		L->logical_length--;
	}
}

bool list_contains(list* L, void* data)
{
	bool result = false;

	assert(L != NULL && L->compare_fn != NULL);

	node* p = L->head->next;

	while (p->next != NULL)
	{
		if (L->compare_fn(p->data, data) == 0)
		{
			result = true;
			break;
		}
		p = p->next;
	}

	return result;
}
