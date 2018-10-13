/*
 * list.h
 *
 *	[NULL]<-[head]<->[Xn]<->[tail]->[NULL]
 *
 *  Created on: Jul 26, 2018
 *      Author: Juniper
 */

#ifndef LIST_H_
#define LIST_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct node node;
typedef struct list list;

typedef void (*free_function)(void *);
typedef int (*compare_function)(const void*, const void*);

struct node {
	void* data;
	node* prev;
	node* next;
};

struct list {
	node* head;
	node* tail;
	int logical_length;
	size_t element_size;
	compare_function compare_fn;
	free_function free_fn;

};

list* list_init(list* L, size_t element_size, free_function free_fn, compare_function compare_fn);
node* list_front(list* L);
node* list_back(list* L);
bool list_contains(list* L, void* data);
void list_push_front(list* L, void* data);
void list_push_back(list* L, void* data);
void list_pop_front(list* L);
void list_pop_back(list* L);
void list_remove_node(list* L, node* p);
void list_remove_pid(list* L, void* data);

#endif /* LIST_H_ */
