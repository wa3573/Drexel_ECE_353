/* This program illustrates the use of the UNIX pipe construct to perform inter-process communication (IPC)
 * between a parent process and its child.
 *
 * Compile the code as follows: gcc -o simple_pipe simple_pipe.c -std=c99 -Wall
 *
 * The user enters a string as a command-line argument to the program. The parent process creates the pipe, forks a child, and passes
 * this string via the pipe for the child to print.
 *
 * Author: Naga Kandasamy
 * Date created: December 22, 2008
 * Date modifed: Jluy 4, 2018
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define BUF_LEN 256

int
main (int argc, char **argv)
{
    int pid;
    int fd[2];    /* Array to hold the read and write file descriptors for the pipe. */
    int n;
    char buffer[BUF_LEN];
    int status;

    if (argc < 2) {             /* Check if we have command line arguments. */
      printf ("Usage: %s <string> \n", argv[0]);
      exit (EXIT_SUCCESS);
    }

    if (pipe (fd) < 0) {        /* Create the pipe data structure. */
        perror ("pipe");
        exit (EXIT_FAILURE);
    }

    if ((pid = fork ()) < 0) {  /* Fork the parent process. */
        perror ("fork");
        exit (EXIT_FAILURE);
    }

    if (pid > 0) {                                                              /* Parent code */
        strcpy (buffer, argv[1]);                                               /* Copy the input string into buffer */
        close (fd[0]);                                                          /* Close the reading end of parent pipe */
        printf ("PARENT: Writing %d bytes to the pipe: \n", (int) strlen(buffer));
        write (fd[1], buffer, strlen(buffer));                                  /* Write the buffer contents to the pipe */
    }
    else {                                                                      /* Child code */
        close (fd[1]);                                                          /* Close writing end of child pipe */
        n = read (fd[0], buffer, BUF_LEN);                                      /* Read n bytes from the pipe */
        buffer[n] = '\0';                                                       /* Terminate the string */
        printf ("CHILD: %s \n", buffer);
        exit (EXIT_SUCCESS);                                                    /* Child exits */
    }

    /* Parent code */
    pid = waitpid (pid, &status, 0);                                            /* Wait for child to terminate */
    printf ("PARENT: Child has terminated. \n");
    exit (EXIT_SUCCESS);
}
