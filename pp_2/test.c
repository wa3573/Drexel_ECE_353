/*
 * test.c
 *
 *  Created on: Aug 24, 2018
 *      Author: Juniper
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int main(int argc, char** argv)
{
	queue_t* q;
	queue_element_t* q_el;
	int i;

	q = create_queue();

	for (i = 0; i < 256; i++)
	{
		char out[32];

		sprintf(out,"%d", i);
		q_el = (queue_element_t*)malloc(sizeof(queue_element_t));
		strcpy(q_el->path_name, out);
		printf("inserting item %d \n", i);
		insert_element(q, q_el);
	}

	printf("number of items in queue: %d \n", num_elements(q));
}
