/*
 * client.c
 *
 *  Created on: Jul 25, 2018
 *      Author: Juniper
 */


#include "fifo_seqnum.h"

static char client_fifo[CLIENT_FIFO_NAME_LEN];

static void remove_fifo(void)
{
	unlink(client_fifo);
}

int main(int argc, char** argv)
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0)
	{
		printf("Usage: %s <seq-len> \n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	unmask(0);
	snprintf(client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)getpid());
	printf("CLIENT: creating FIFO %s \n", client_fifo);

	if (mkfifo (client_fifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
	{
		perror("mkfifo");
		exit(EXIT_FAILURE);
	}

    if (atexit (remove_fifo) != 0){
        perror ("atexit");
        exit (EXIT_FAILURE);
    }

    int server_fd, client_fd;
    struct request req;
    struct response resp;

    server_fd = open (SERVER_FIFO, O_WRONLY);
    if (server_fd == -1){
        printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
        exit (EXIT_FAILURE);
    }

    /* Send the message to the server */
    if (write (server_fd, &req, sizeof (struct request)) != sizeof (struct request)){
        printf ("Cannot write to server");
        exit (EXIT_FAILURE);
    }

    /* Open our FIFO and read the response from the server. */
    client_fd = open (client_fifo, O_RDONLY);
    if (client_fd == -1){
        printf ("Cannot open FIFO %s for reading \n", client_fifo);
        exit (EXIT_FAILURE);
    }

    if (read(client_fd, &resp, sizeof (struct response)) != sizeof (struct response)){
        printf ("Cannot read response from server \n");
        exit (EXIT_FAILURE);
    }

    printf ("Response received from server: %d \n", resp.seq_num);
    exit (EXIT_SUCCESS);
}
