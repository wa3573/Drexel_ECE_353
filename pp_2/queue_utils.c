/* Helper functions for queue operations. 
 * 
 * Author: Naga Kandasamy
 * Date: 17 August 2018
 *
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"


queue_t *	/* Creates the queue data structure. */
create_queue (void)
{
    queue_t *this_queue = (queue_t *) malloc (sizeof (queue_t)); 
	if(this_queue == NULL) return NULL;
	this_queue->head = this_queue->tail = NULL;
	return this_queue;
}

void 	/* Insert an element in the queue. */
insert_element (queue_t *queue, queue_element_t *element)
{
	element->next = NULL;

	if(queue->head == NULL){ /* Queue is empty */
		queue->head = element;
		queue->tail = element;
	} else{ // Add element to the tail of the queue
		(queue->tail)->next = element;
		queue->tail = (queue->tail)->next;
	}
}

queue_element_t *	/* Remove element from the head of the queue. */
remove_element(queue_t *queue)
{
	queue_element_t *element = NULL; 
	if(queue->head != NULL){
		element = queue->head;
		queue->head = (queue->head)->next;
	}
	return element;
}

void 
print_queue(queue_t *queue) /* Print the elements in the queue. */
{
	queue_element_t *current = queue->head;
	while(current != NULL){
		printf("[ %s ]->\n", current->path_name);
		current = current->next;
	}

	printf("NULL\n");
}

int num_elements(queue_t *queue) /* Print the elements in the queue. */
{
	queue_element_t *current = queue->head;
	int count = 0;
	while(current != NULL){
		count++;
		current = current->next;
	}

	return count;
}
