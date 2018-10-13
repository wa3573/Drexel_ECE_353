/*
 * main.c
 *
 *  Created on: Jun 28, 2018
 *      Author: Juniper
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int value = 5;

int main (void)
{
	pid_t pid;
	pid = fork ();

	if (pid == 0){ /* Child code */
		value += 15;
		return 0;
	}
	else if (pid > 0){ /* Parent code */
		wait (NULL);
		printf ("PARENT: value = %d \n", value);
		return 0;
	}
}
