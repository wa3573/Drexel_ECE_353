/* Main program that uses pipes to connect the three filters as follows:
 *
 * ./gen_numbers n | ./filter_pos_nums | ./calc_avg
 *
 * Author: William Anderson
 * Date created: 7/15/2018
 *
 * Compile as follows:
 * gcc -o pipes_gen_filter_avg pipes_gen_filter_avg.c -std=c99 -Wall -lm
 * */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_BUFFER_SIZE 256
#define STDIN 0
#define STDOUT 1

int child_setup_pipe(int fd0[2], int fd1[2])
{
	  close(fd0[1]); // Close the writing end of fd0
	  dup2(fd0[0], STDIN);
	  close(fd0[0]);
	  close(fd1[0]); // Close the reading end of fd1
	  dup2(fd1[1], STDOUT); // Make the standard output descriptor (STDOUT) refer to fd1[1], the writing end of pipe fd1
	  close(fd1[1]);

	  return 0;
}

int main (int argc, char **argv)
{
	int fd0[2], fd1[2], fd2[2], fd3[2];

	char* buffer = (char*) malloc(sizeof(char) * MAX_BUFFER_SIZE);
	int n;
	pid_t pid = 0;
	pid_t wait_pid;
	int status = 0;

	if(argc < 2){
	  printf("Usage: pipes_gen_filter \"number\" \n");
	  exit(EXIT_SUCCESS);
	}

	if(pipe(fd0) < 0 || pipe(fd1) < 0){
		printf("Cannot create the pipe. Exiting the program. \n");
	  exit(EXIT_FAILURE);
	}

	pid = fork();
	if(pid == 0) { // Child #1
		close(fd0[1]); // Close the writing end of fd0
		close(fd1[0]); // Close the reading end of fd1
		dup2(fd0[0], STDIN); // Make the standard input descriptor (STDIN) refer to fd0[0], the reading end of pipe fd0
		dup2(fd1[1], STDOUT); // Make the standard output descriptor (STDOUT) refer to fd1[1], the writing end of pipe fd1
		close(fd1[1]);

		n = read(fd0[0], buffer, MAX_BUFFER_SIZE);
		buffer[n] = '\0';
		close(fd0[0]); // Close fd0[0]4

		  if (execl("./gen_numbers", "gen_numbers", buffer, (char *) NULL) < 0) // Execute the add2 program without any command-line arguments
			  printf("Child: error executing the gen_numbers coprocess. \n");

	  }

	  if(pipe(fd2) < 0){
		  printf("Cannot create the pipe. Exiting the program. \n");
		  exit(EXIT_FAILURE);
	  }

	  pid = fork();
	  if(pid == 0) { // Child #2
		  close(fd0[0]);
		  close(fd0[1]);
//
		  close(fd1[1]); // Close the writing end of fd0
		  close(fd2[0]); // Close the reading end of fd1
		  dup2(fd1[0], STDIN); // Make the standard input descriptor (STDIN) refer to fd0[0], the reading end of pipe fd0
		  dup2(fd2[1], STDOUT); // Make the standard output descriptor (STDOUT) refer to fd1[1], the writing end of pipe fd1
		  close(fd2[1]);
		  close(fd1[0]);

		  if (execl("./filter_pos_nums", "filter_pos_nums", (char *) NULL) < 0) // Execute the add2 program without any command-line arguments
			  printf("Child: error executing the gen_numbers coprocess. \n");
	  }

	  if(pipe(fd3) < 0){
		  printf("Cannot create the pipe. Exiting the program. \n");
		  exit(EXIT_FAILURE);
	  }

	  pid = fork();
	  if(pid == 0) { // Child #3
		  close(fd0[0]);
		  close(fd0[1]);
		  close(fd1[0]);
		  close(fd1[1]);

		  close(fd2[1]); // Close the writing end of fd2
		  close(fd3[0]); // Close the reading end of fd3
		  dup2(fd2[0], STDIN); // Make the standard input descriptor (STDIN) refer to fd2[0], the reading end of pipe fd0
		  dup2(fd3[1], STDOUT); // Make the standard output descriptor (STDOUT) refer to fd3[1], the writing end of pipe fd1
		  close(fd3[1]);
		  close(fd2[0]);

		  if (execl("./calc_avg", "calc_avg", (char *) NULL) < 0) // Execute the add2 program without any command-line arguments
			  printf("Child: error executing the gen_numbers coprocess. \n");

	  }

		close(fd0[0]); // Close the reading end of pipe 0
		close(fd1[0]); // Close the reading end of pipe 1
		close(fd1[1]); // Close the writing end of pipe 1
		close(fd2[0]); // Close the reading end of pipe 2
		close(fd2[1]); // Close the writing end of pipe 2
		close(fd3[1]); // Close the writing end of pipe 3

		/* Read the command line argument. */
		strcpy(buffer, argv[1]); // Read in the string input by the user

		/* Send to 1st child */

		n = write(fd0[1], buffer, strlen(buffer)); // Write strlen(buffer) bytes to the writing end of fd0
		close(fd0[1]); // Close the writing end of pipe 0

		while ((wait_pid = wait(&status)) > 0); // Wait for children to finish

		/* Read from 3rd child */

		n = read(fd3[0], buffer, MAX_BUFFER_SIZE); // Read n bytes from the reading end of fd1
		close(fd3[0]);  // Close the reading end of pipe 3
		buffer[n] = '\0'; // Terminate the string you just read

		printf("%s", buffer);

		free(buffer);
		exit (EXIT_SUCCESS);
}

