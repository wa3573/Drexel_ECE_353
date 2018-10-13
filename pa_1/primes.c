 /* Skeleton code from primes.
  *
  * Author: Naga Kandasamy / William Anderson
  *
  * Date created: June 28, 2018
  * Date updated: July 2, 2018
  *
  * Build your code as follows: gcc -o primes primes.c -std=c99 -Wall
  *
  * */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>

#define FALSE 0
#define TRUE !FALSE

typedef struct {
	int* buffer;
	int* buffer_end;
	void* head;
	void* tail;
	size_t size;
	size_t count;
	size_t capacity;
} circular_buffer;

unsigned long int num_found; /* Number of prime numbers found */
circular_buffer buff;

void cb_init(circular_buffer* cb, size_t capacity, size_t size)
{
	cb->buffer = malloc(capacity * size);
	if(cb->buffer == NULL)
		printf("Error, could not allocate buffer \n");
	cb->buffer_end = ( int* )cb->buffer + capacity + size;
	cb->count = 0;
	cb->size = size;
	cb->capacity = capacity;
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
}

void cb_free(circular_buffer* cb)
{
	free(cb->buffer);
	free(cb->buffer_end);
	free(cb->head);
	free(cb->tail);
	free(( size_t* )cb->size);
	free(( size_t* )cb->count);
	free(( size_t* )cb->capacity);
}

int cb_push_back(circular_buffer* cb, unsigned long int* item)
{
	int error = 0;
	if (cb->count == cb->capacity) {
		error = -1; // Buffer is full
	}

	memcpy(cb->head, item, cb->size);
	cb->head = ( char* )cb->head + cb->size;

	if (cb->head == cb->buffer_end) {
		cb->head = cb->buffer;
	}

	cb->count++;
	return error;
}

int cb_pop_front(circular_buffer* cb, void* item)
{
	int error = 0;
	if (cb->count == 0) {
		error = -1; // Buffer is empty
		return error;
	}

	memcpy(item,cb->tail, cb->size);
	cb->tail = ( char* )cb->tail + cb->size;

	if (cb->tail == cb->buffer_end)
		cb->tail = cb->buffer;
	cb->count--;
	return error;
}

int report (circular_buffer* cb)
{
	int i, num, error;
	error = 0;

	printf("   Last 5 Found Primes: \n");

	for (i = 0; i < 5; i++) {
		if (cb_pop_front(cb, &num)) {
			error = -1;
			printf("Error, buffer empty \n");
			return error;
		}
		printf(" [%d] ", num);
	}

	printf("\n");
	return error;
}

void int_handler(int sig)
{
	signal(sig, SIG_IGN);
	printf("\nSIGINT detected, quitting... \n");
	report(&buff);
	exit(0);
}

void quit_handler(int sig)
{
	signal(sig, SIG_IGN);
	printf("\nSIGQUIT detected, quitting... \n");
	report(&buff);
	exit(0);
}

int is_prime(unsigned int num)
{
    unsigned long int i;

	if (num < 2) {
		return FALSE;
	} else if (num == 2) {
		return TRUE;
	} else if (num % 2 == 0) {
		return FALSE;
	} else {
		for (i = 3; (i*i) <= num; i += 2) {
			if (num % i == 0) {
				return FALSE;
			}
		}
		return TRUE;
	}
}

int main (int argc, char** argv)
{
    unsigned long int num;

    num_found = 0;

    cb_init(&buff, 5, sizeof(int));
    signal(SIGINT, int_handler);
    signal(SIGQUIT, quit_handler);

    printf ("Beginning search for primes between 1 and %lu. \n", LONG_MAX);
    for (num = 1; num < LONG_MAX; num++) {
		if (is_prime (num)) {
			num_found++;
			cb_push_back(&buff, &num);
			printf("%lu \n", num);
		}
        // sleep (1);
    }

    return 0;
}
