/* Echo Chamber
 *
 * Author: William Anderson
 * Date created: 7/15/2018
 *
 * Compile as follows:
 * gcc -o echo_chamber echo_chamber.c -std=c99 -Wall
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_BUFFER_SIZE 256
#define STDIN 0
#define STDOUT 1
#define TRUE 1
#define FALSE 0

static volatile sig_atomic_t int_flag = 0;
static volatile pid_t pid;
int fd0[2], fd1[2];

int create_sync_channel (void)          /* Create the communication channel */
{
	if (pipe (fd0) < 0 || pipe (fd1) < 0)
    	return FALSE;

	return TRUE;
}

void get_text(FILE* fp, char* buffer)
{
	int c;
	int i = 0;

    while((c = fgetc(fp)) != EOF)
    {
    	buffer[i] = c;
    	i++;
    }

    buffer[i] = '\0';
}

void get_upper(char* in, char* out)
{
    int i = 0;

	while(in[i])
    {
    	out[i] = toupper(in[i]);
    	i++;
    }
}

void int_handler(int sig)
{
	signal(sig, SIG_IGN);

	if (pid)
	{
		printf("\nSIGINT detected, quitting on next iteration... \n");
	}

	int_flag = 1;
}

void quit_handler(int sig)
{
	signal(sig, SIG_IGN);

	if (pid)
	{
		printf("\nSIGQUIT detected, quitting on next iteration... \n");
	}

	int_flag = 1;
}


int main(int argc, char** argv)
{
	char* buffer;
	char* upper;

	int n;
	pid_t wait_pid;
	int status = 0;

    signal(SIGINT, int_handler);
    signal(SIGQUIT, quit_handler);


	buffer = (char*) calloc(MAX_BUFFER_SIZE + 1, sizeof(char));
	if (buffer == NULL)
	{
		printf("Error: can't allocate memory \n");
		exit(EXIT_FAILURE);
	}

	upper = (char*) calloc(MAX_BUFFER_SIZE + 1, sizeof(char));
	if (upper == NULL)
	{
		printf("Error: can't allocate memory \n");
		exit(EXIT_FAILURE);
	}

	while(int_flag == 0)
	{
		if((pipe(fd0) < 0) || (pipe(fd1) < 0)){ 	// Create two pipes
			printf("Cannot create the pipes. Exiting the program. \n");
			exit(-1);
		}

		switch(pid = fork()){
			case -1:
				printf("Error forking child process. \n");
				exit(-1);

			case 0: 								/* Child Process */
				close(fd0[1]);
				close(fd1[0]);

				n = read(fd0[0], buffer, MAX_BUFFER_SIZE);
				close(fd0[1]);
				buffer[n] = '\0';

				get_upper(buffer, upper);

				n = write(fd1[1], upper, MAX_BUFFER_SIZE);
				close(fd1[1]);

				free(buffer);
				free(upper);

				exit(EXIT_SUCCESS);

			default: 								/* Parent Process */
				get_text(stdin, buffer);

				close(fd0[0]);
				close(fd1[1]);

				n = write(fd0[1], buffer, strlen(buffer));
				close(fd0[1]);

				while ((wait_pid = wait(&status)) > 0); // Wait for children to finish

				n = read(fd1[0], buffer, MAX_BUFFER_SIZE);
				close(fd1[0]);
				buffer[n] = '\0';

				printf("%s", buffer);

		}
	}

	free(buffer);
	free(upper);

	exit(EXIT_SUCCESS);
}
