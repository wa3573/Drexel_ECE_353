/* Program creates a shared memory area in the address space of the parent process that the child can access. In effect, the child and
 * parent share that area of memory, and can communicate with each other via the shared-memory area.
 *
 * Author: Naga Kandasamy
 * Date created: Jamuary 14, 2009
 * Date modified: July 16, 2018
 *
 * Compile as follows:
 * gcc -o shared_memory shared_memory.c -std=c99 -Wall
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

#define BUF_LEN 256
#define NUM_ITEMS 10
#define SIZE_OF_FIRST_SHARED_MEMORY NUM_ITEMS * sizeof(int)   /* Size of the shared memory area; it can store 10 integers */
#define SIZE_OF_SECOND_SHARED_MEMORY 256 * sizeof(char) /* Size of shared memory area; it can store 256 characters */


int main (int argc, char **argv)
{

    int flags;
    flags = O_CREAT;
    mode_t perms = S_IRUSR | S_IWUSR;

//    unsigned int sem_value = 0;
    sem_t* sem_1 = sem_open("wja35_pc_sem_1", flags, perms, 0);
    sem_t* sem_2 = sem_open("wja35_pc_sem_2", flags, perms, 0);
    sem_t* sem_3 = sem_open("wja35_pc_sem_3", flags, perms, 0);

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


    printf("Server: wait(sem_3) - waiting for shared memory access\n");
	// shared memory access  wait
	if (sem_wait(sem_3) == -1)
	{
        perror ("sem_wait");
        exit (EXIT_FAILURE);
	}

	printf("Server: got shared memory access!\n");


    exit (EXIT_SUCCESS);
}
