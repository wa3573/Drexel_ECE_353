/* The producer code. The producer generates work items and places them in the 
 * shared buffer. 
 *
 * Author: Naga Kandasamy
 * Date created: August 6, 2018
 *
 * Last edited by:
 * William Anderson, 8/23/2018
 *
 * Compile as follows: 
 * gcc -o producer producer.c -std=c99 -Wall -lm 
 *
 * Run the producer program before starting any of the consumer programs. 
 *
 * */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdbool.h>
#include "buffer.h"

static volatile sig_atomic_t flag_int = false;
static bool VERBOSE = false;

void initialize_sems()
{
    /* Semaphores
     * 			value	condition
     * sem_1	0		work buffer is full, do not continue (initial)
     * sem_1	1		work buffer is not full, continue
     * sem_2	0		work is being generated (initial)
     * sem_2	1		work is ready
     * sem_3	0		shared data being used
     * sem_3 	1		shared data is free		(initial)
     *
     * note: sem_1 default is 0, causing program to block on first call to
     * sem_wait(sem_1), this is intentional, so it may be unlocked by
     * the consumer.
     */

	int flags;
    flags = O_CREAT;
    mode_t perms = S_IRUSR | S_IWUSR;

    sem_1 = sem_open(sem_1_name, flags, perms, 0);
    sem_2 = sem_open(sem_2_name, flags, perms, 0);
    sem_3 = sem_open(sem_3_name, flags, perms, 1);

    if (sem_1 == SEM_FAILED) {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }

    if (sem_2 == SEM_FAILED) {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }

    if (sem_3 == SEM_FAILED) {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }
}

buffer* initialize_shared_mem()
{
    int fd;
	buffer* buf_out;

	/* Create the shared memory object using shm_open() */
    fd = shm_open (SHARED_OBJECT_PATH, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
    if (fd < 0) {
        perror ("shm_open()");
        exit (EXIT_FAILURE);
    }
    if (VERBOSE)
    	fprintf (stderr, "Created shared memory object %s\n", SHARED_OBJECT_PATH);

    /* Adjust mapped file size (make room for the whole segment to map) using ftruncate(). */
    ftruncate (fd, SIZE_OF_SHARED_MEMORY);

    /* Request the shared segment using mmap(). */
    buf_out = (buffer *) mmap (NULL, SIZE_OF_SHARED_MEMORY, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf_out == NULL) {
        perror ("mmap()");
        exit (EXIT_FAILURE);
    }

    if (VERBOSE)
    	fprintf (stderr, "Shared memory segment allocated correctly (%d bytes).\n", (int)SIZE_OF_SHARED_MEMORY);

    return buf_out;
}

void initialize_buffer(buffer* buf)
{
    buf->in = 0;
    buf->out = 0;
    buf->is_finished = false;
    buf->is_full = false;
}

void* create_shared_memory (int size_of_shared_memory)
{
    int fd;
    void *area;

    fd = open("/dev/zero", O_RDWR);
    area = mmap(NULL, size_of_shared_memory, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    close(fd);
    return area;
}

/* Returns a random number between min and max from a uniform distribution to simulate work 
 * for the client */
int produce_work_item (int min, int max)
{
    return((int)floor((double)(min + (max - min + 1)*((float)rand()/(float)RAND_MAX))));
}

/* Returns a random number between min and max from a uniform distribution to simulate the  
 * resting time for the producer before moving on to the next work item */
int get_rest_time (int min, int max)
{
     return((int)floor((double)(min + (max - min + 1)*((float)rand()/(float)RAND_MAX))));
}

void lock_shared_mem()
{
	if (VERBOSE)
	{
		printf("Server: lock_shared_mem() - locking shared memory\n");
	}

	// shared memory access  wait
	if (sem_wait(sem_3) == -1)
	{
        perror ("sem_wait");
        exit (EXIT_FAILURE);
	}
}

void unlock_shared_mem()
{
	if (VERBOSE)
	{
		printf("Server: unlock_shared_mem() - releasing shared memory\n");
	}

	// shared memory access post
	if (sem_post(sem_3) == -1)
	{
        perror ("sem_wait");
        exit (EXIT_FAILURE);
	}
}


void wait_for_consumer()
{
	if (VERBOSE)
	{
		printf("Server: wait_for_consumer()\n");
	}

	// producer semaphore wait
	if (sem_wait(sem_1) == -1)
	{
        perror ("sem_wait");
        exit (EXIT_FAILURE);
	}
}

void signal_consumer()
{
	if (VERBOSE)
	{
		printf("Server: signal_consumer()\n");
	}

    // consumer semaphore post (signal the consumer that item is ready)
    if (sem_post(sem_2) == -1)
    {
    	perror("sem_post");
    	exit(EXIT_FAILURE);
    }
}

void unlink_all()
{
    if (sem_unlink (sem_1_name) == -1)
    {
        perror ("sem_unlink");
        exit (EXIT_FAILURE);
    }

    if (sem_unlink (sem_2_name) == -1)
    {
        perror ("sem_unlink");
        exit (EXIT_FAILURE);
    }

    if (sem_unlink (sem_3_name) == -1)
    {
        perror ("sem_unlink");
        exit (EXIT_FAILURE);
    }

    if (shm_unlink (SHARED_OBJECT_PATH) != 0) {
        perror ("shm_unlink()");
        exit (EXIT_FAILURE);
    }
}


int main (int argc, char **argv)
{
	buffer* mem_shared_buffer;

	remove("/dev/shm/sem.wja35_pc_sem_1");
	remove("/dev/shm/sem.wja35_pc_sem_2");
	remove("/dev/shm/sem.wja35_pc_sem_3");
	remove("/dev/shm/wja35_buff_shm");

    if (argc != 2){
        printf ("Usage: %s num-items \n", argv[0]);
        exit (EXIT_FAILURE);
    }

    /* The maximum number of items that will be produced is specified via 
     * the command-line argument
     * */
    int num_items = atoi (argv[1]);
    
    /* Seed the random number generator */
    srand (time (NULL));

    mem_shared_buffer = initialize_shared_mem();
    initialize_sems();

    int i, item;
    int rest_time;

    bool finished;
    bool is_full;

    /* initialize buffer */
    /* Memory lock */
    lock_shared_mem();
    initialize_buffer(mem_shared_buffer);
    /* Memory unlock */
    unlock_shared_mem();

    /* Loop to produce NUM_ITEMS work items for the consumers */
    for (i = 0; i < num_items; i++)
    {
    	// Break on interrupt flag
    	if (flag_int)
    		break;

    	printf("Server: producing work item #%d\n", i + 1);
        /* Produce a work item */
        item = produce_work_item (MIN_WORK_TIME, MAX_WORK_TIME);

        /* Memory lock */
        lock_shared_mem();

        /* If buffer is full, unlock the shared memory and wait for the
         * consumer to signal that it has consumed an item
         */
        if (mem_shared_buffer->in == BUF_SIZE)
        {
        	is_full = true;
        	mem_shared_buffer->is_full = is_full;

        	/* Memory unlock */
            unlock_shared_mem();
            printf("Server: buffer is full, waiting for consumer \n");
            wait_for_consumer();
            /* Memory lock */
        	lock_shared_mem();
        }

        /* Add work item, increment in and out.  */
        mem_shared_buffer->work[mem_shared_buffer->in] = item;
        mem_shared_buffer->out = mem_shared_buffer->in;
        (mem_shared_buffer->in)++;

        signal_consumer();
        /* Memory unlock */
        unlock_shared_mem();

    	printf("Server: sleeping...\n");
        /* Rest for some time before producing next work item */
        rest_time = get_rest_time (MIN_REST_TIME, MAX_REST_TIME);
        sleep (rest_time);
    }

    printf("Production completed, waiting for all items to be consumed...\n");

    /* Check every 10 msec if "in" has decremented back to zero, meaning all items have been
     * consumed
     */
    while(!finished && !flag_int)
    {
    	/* Memory lock */
    	lock_shared_mem();

    	/* Check if all items have been consumed */
        if (mem_shared_buffer->in == 0)
        {
        	/* Update buffer to reflect finished state */
        	mem_shared_buffer->is_finished = true;
        	finished = true;

        	/* Post to signal_consumer semaphore to make sure no clients get stuck
        	 * waiting for production. (Done once for each possible consumer, for
        	 * simplicity)
        	 */
            for(i = 0; i < MAX_NUM_CONSUMERS; i++)
            {
            	signal_consumer();
           	}
        }

        /* Memory unlock */
        unlock_shared_mem();
    	usleep (1E4);
    }

    if (finished)
    	printf("Server: all work consumed, waiting %d seconds for consumers...\n", (int)(1.5 * MAX_WORK_TIME));
    else
    	printf("Server: exiting, waiting %d seconds for consumers...\n", (int)(1.5 * MAX_WORK_TIME));

    /* Wait for slightly longer than MAX_WORK_TIME for consumers to close */
    sleep(1.5 * MAX_WORK_TIME);

    /* Unlink all semaphores, unlink shared memory */
    if (VERBOSE)
    {
    	printf("Server: unlinking semaphores and shared memory\n");
    }
    unlink_all();

    printf("Exiting...\n");
       
    exit (EXIT_SUCCESS);
}
