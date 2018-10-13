/*
 * consume.c
 *
 *  Created on: Aug 20, 2018
 *      Author: Juniper
 */

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

int main(int argc, char** argv)
{
    sem_t* sem_1;
    sem_t* sem_2;
    sem_t* sem_3;

    sem_1 = sem_open ("wja35_pc_sem_1", 0);
    if (sem_1 == SEM_FAILED)
    {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }

    sem_2 = sem_open ("wja35_pc_sem_3", 0);
    if (sem_2 == SEM_FAILED)
    {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }

    sem_3 = sem_open ("wja35_pc_sem_3", 0);
    if (sem_3 == SEM_FAILED)
    {
        perror ("sem_open");
        exit (EXIT_FAILURE);
    }

    printf("Client: post(sem_3) - releasing shared memory\n");
    // post(sem_3): release shared memory
    if (sem_post (sem_3) == -1)
    {
    	perror("sem_post");
    	exit(EXIT_FAILURE);
    }
}
