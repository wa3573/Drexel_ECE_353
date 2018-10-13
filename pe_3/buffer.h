#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdbool.h>

#define NUM_ITEMS 100
#define BUF_SIZE 25
#define MAX_NUM_CONSUMERS 100


/* Some values to simulate the time taken by the producer to produce the 
 * work items and for the consumer to consume the work items 
 * */
#define MIN_WORK_TIME 1
#define MAX_WORK_TIME 8

#define MIN_REST_TIME 1
#define MAX_REST_TIME 1
#define SHARED_OBJECT_PATH         "/wja35_buff_shm"
#define SIZE_OF_SHARED_MEMORY sizeof(buffer)   /* Size of the shared memory area */

/* The buffer structure that is known to both the producer and the consumer. */
typedef struct buffer {
    int work[BUF_SIZE]; /* Number of slots in the buffer to place work */
    int in;             /* The index within the buffer which the producer uses to place the work */
    int out;            /* The index within the buffer which the consumer uses to consume the work */
    bool is_full;
    bool is_finished;
} buffer;

char sem_1_name[32] = "wja35_pc_sem_1";
char sem_2_name[32] = "wja35_pc_sem_2";
char sem_3_name[32] = "wja35_pc_sem_3";
sem_t* sem_1;
sem_t* sem_2;
sem_t* sem_3;


#endif
