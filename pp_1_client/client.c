/*
 * 	Chat Room: Client
 *
 * 	adapted from fifo_seqnum_client.c
 *
 *  Created on: Jul 11, 2018
 *      Author: William Anderson
 */

#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#include "chat.h"
#include "style.h"

#define STD_LEN 1024
#define MAIN_LOOP_SLEEP_MS 100

static char client_fifo[CLIENT_FIFO_NAME_LEN];
static volatile sig_atomic_t int_sys_flag = false;

static void exit_handler (void);
void int_sys_handler(int sig);
void req_format_check_connection(struct request* req);
void req_format_connect(struct request* req);
void req_format_disconnect(struct request* req);
void req_format_message(struct request* req);
bool check_sys_response(struct response* resp);
void get_text_EOF(FILE* fp, char* buffer);
bool fifo_is_empty(int fd);
bool client_connect(struct request* req, struct response* resp);
bool client_disconnect(struct request* req, struct response* resp);
bool client_send(struct request* req, struct response* resp);
bool client_read_messages(struct request* req, struct response* resp);

/* Exit handler for the program. */
static void exit_handler (void)
{
	unlink (client_fifo);
}

// Signal handler for SIGINT and SIGQUIT
void int_sys_handler(int sig)
{
//	bool conn_success = false;
//	struct request req;
//	struct response resp;
//
	printf("\n---- SIG detected, closing connection ----\n");

	fclose(stdin);
	int_sys_flag = true;

}

// Format request for checking connection
void req_format_check_connection(struct request* req)
{
    req->dest_pid = 0;
    req->global = false;
    req->system = true;
    strcpy(req->message, "check_connection");
}

// Format request for client connect
void req_format_connect(struct request* req)
{
	req->dest_pid = 0;
    req->global = false;
    req->system = true;
    strcpy(req->message, "new_client");
}

// Format request for client disconnect
void req_format_disconnect(struct request* req)
{
    req->dest_pid = 0;
    req->global = false;
    req->system = true;
    strcpy(req->message, "disconnect_client");
}

// Acquire user input and format request
void req_format_message(struct request* req)
{
	char* message;
	char* choice;
	bool flag_cont = false;

	message = (char*)malloc(sizeof(char) * MESSAGE_LENGTH);
	choice = (char*)malloc(sizeof(char) * STD_LEN);

	req->dest_pid = 0;
	req->global = false;
	req->system = false;

	printf("%sReading until EOF, press CTRL+D to end message... %s\n", T_ITAL_ON, T_RESET);

	get_text_EOF(stdin, message);
	strcpy(req->message, message);

	while (flag_cont == false && !int_sys_flag)
	{
    	printf("\n %sGlobal message? ['y' or 'n']%s ", T_ITAL_ON, T_RESET);
    	fgets(choice, STD_LEN, stdin);

		if (strcmp(choice, "y\n") == 0)
		{
			req->global = true;
			req->dest_pid = 0;
			flag_cont = true;
		} else if (strcmp(choice, "n\n") == 0) {
			req->global = false;

	    	printf("\n %sEnter destination pid:%s ", T_ITAL_ON, T_RESET);
	    	fgets(choice, STD_LEN, stdin);

	    	req->dest_pid = atoi(choice);
	    	flag_cont = true;
		} else {
			printf("%sPlease enter either ['y' or 'n']%s", T_ITAL_ON, T_RESET);
		}
	}

	free(message);
	free(choice);
}

bool check_sys_response(struct response* resp)
{
	int client_fd;
	/* Open our FIFO and read the response from the server. */
	client_fd = open (client_fifo, O_RDONLY);
	if (client_fd == -1){
		printf ("Cannot open FIFO %s for reading \n", client_fifo);
		return false;
	}

	if (read(client_fd, resp, sizeof (struct response)) != sizeof (struct response)){
		printf ("Cannot read response from server \n");
		return false;
	}

	if (strcmp(resp->message, "OK") == 0)
	{
		return true;
	} else {
		return false;
	}

}

// Get user input until EOF
void get_text_EOF(FILE* fp, char* buffer)
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

// Check if FIFO holds data
bool fifo_is_empty(int fd)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;

	if (poll(&pfd, 1, 0) == 1)
	{
		return false;
	}

	return true;
}

bool client_connect(struct request* req, struct response* resp)
{
    int server_fd;

    req_format_connect(req);

    server_fd = open (SERVER_FIFO, O_WRONLY);
	if (server_fd == -1){
		printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
		return false;
	}

	/* Send the message to the server */
	if (write (server_fd, req, sizeof (struct request)) != sizeof (struct request)){
		printf ("Cannot write to server");
		return false;
	}

	if (check_sys_response(resp))
	{
		return true;
	} else {
		return false;
	}

}

bool client_disconnect(struct request* req, struct response* resp)
{
    int server_fd;

    req_format_disconnect(req);

    server_fd = open (SERVER_FIFO, O_WRONLY | O_NONBLOCK);
	if (server_fd == -1){
		printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
		return false;
	}

	/* Send the message to the server */
	if (write (server_fd, req, sizeof (struct request)) != sizeof (struct request)){
		printf ("Cannot write to server");
		return false;
	}

	if (check_sys_response(resp))
	{
		return true;
	} else {
		return false;
	}

}

// Send request to server
bool client_send(struct request* req, struct response* resp)
{
    int server_fd;

	server_fd = open (SERVER_FIFO, O_WRONLY | O_NONBLOCK);
	if (server_fd == -1){
		printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
		return false;
	}

	/* Send the message to the server */
	if (write (server_fd, req, sizeof (struct request)) != sizeof (struct request)){
		printf ("Cannot write to server");
		return false;
	}

	if (check_sys_response(resp))
	{
		return true;
	} else {
		return false;
	}

}

// Read client FIFO
bool client_read_messages(struct request* req, struct response* resp)
{
	int client_fd;
	bool flag_msg_recieved = false;

	client_fd = open (client_fifo, O_RDONLY | O_NONBLOCK);
	if (client_fd == -1){
		printf ("Cannot open FIFO %s for reading \n", client_fifo);
		return false;
	}

	// Read messages while the FIFO still contains data
	while (!fifo_is_empty(client_fd))
	{
		if (read(client_fd, resp, sizeof (struct response)) != sizeof (struct response)){
//			printf ("Cannot read response from server \n");
			flag_msg_recieved = false;
		} else {
			flag_msg_recieved = true;
		}

		if (flag_msg_recieved)
		{
			// Output message with sender and text
			printf("%sMessage from PID%s %s%s%ld%s: \n%s\n",
					T_ITAL_ON, T_RESET,
					T_BOLD_ON, T_C_YEL,
					(long)resp->origin_pid,
					T_RESET, resp->message);
		}
	}

	return true;
}

int main (int argc, char **argv)
{
	struct sigaction int_sys_handle = {.sa_handler = int_sys_handler};
    struct request* req;
    struct response* resp;

    bool conn_success = false;
    bool conn_alive = false;
    bool read_success = false;
    bool send_success = false;
    bool disconn_success = false;

    char msg_sys[STD_LEN];
	char ch = 0;

	sigaction(SIGINT, &int_sys_handle, 0);
	sigaction(SIGQUIT, &int_sys_handle, 0);

    req = (struct request*)malloc(sizeof(struct request));
    resp = (struct response*)malloc(sizeof(struct response));

	printf(T_BG_C_BLU T_C_YEL "\n\t ========== Chat Room 0.1: Client ========== \n\n" T_RESET);

    umask (0);
    snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)getpid());
    if (mkfifo (client_fifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST){
        perror ("mkfifo");
        exit (EXIT_FAILURE);
    }

    if (atexit (exit_handler) != 0){
        perror ("atexit");
        exit (EXIT_FAILURE);
    }

    // Initiate request
    req->origin_pid = getpid();
    req->seq_len = SEQ_LENGTH;

    printf("New client initiated with PID: %s%s%ld%s \n",
    		T_BOLD_ON, T_C_YEL, (long)req->origin_pid, T_RESET);

    // Connect to server
    conn_success = client_connect(req, resp);

	if (conn_success)
	{
		printf("Connected to server %ssuccessfully \n%s",
				T_C_GRN, T_RESET);
		conn_alive = true;
	} else {
		printf("%sError:%s could not connect to server, exiting... \n",
				T_C_RED, T_RESET);
	}

		/* 		BEGIN MAIN LOOP 	*/

    while(!int_sys_flag && conn_success && conn_alive)
    {
    	req_format_check_connection(req);
    	conn_alive = client_send(req, resp);

    	if (!conn_alive)
    	{
    		printf("%sError:%s connection to server lost, exiting... \n",
    				T_C_RED, T_RESET);
    		continue;
    	} else if (int_sys_flag) {
    		continue;
    	}

    	snprintf(msg_sys, STD_LEN, "Enter: '%s%sr%s' to check for messages, '%s%ss%s' to send a message '%s%sq%s' to quit : ",
    			T_BOLD_ON, T_C_CYN, T_RESET,
				T_BOLD_ON, T_C_CYN, T_RESET,
				T_BOLD_ON, T_C_CYN, T_RESET);

    	printf(msg_sys);

		scanf(" %c", &ch); // Get character and ignore whitespace
		getchar(); // Consume pesky newline

		/*Double check for interupt, since scanf() is blocking until it
		 * receives string input from stdin, and the process may be waiting
		 * there
		*/
		if (int_sys_flag) {
		    		continue;
		 }

		if (ch == 0x73)  // Send message
		{
			printf("Checking for messages first... \n");
			if (!client_read_messages(req, resp))
			{
//				printf("No unread messages \n");
			}
			req_format_message(req);
			send_success = client_send(req, resp);

			if (send_success)
			{
				printf("Message sent %ssuccessfully%s \n", T_C_GRN, T_RESET);
			} else {
				printf("Message %sfailed%s \n", T_C_RED, T_RESET);
			}

			continue;

		} else if (ch == 0x72) {	// Read messages
			read_success = client_read_messages(req, resp);

			if (read_success)
			{
				printf("Messages read %ssuccessfully%s \n", T_C_GRN, T_RESET);
			} else {
				printf("Messages read %sfailed%s \n", T_C_RED, T_RESET);
			}

			continue;

		} else if (ch == 0x71) { 	// Quit
			break; // break the main loop
		} else {
			printf("%sPlease enter a valid choice%s... \n", T_C_RED, T_RESET);
		}

    } // End main loop

			/* END MAIN LOOP */

    /* Begin client disconnect */

	if (conn_success && conn_alive)
	{
		req_format_disconnect(req);
		disconn_success = client_disconnect(req, resp);

		if (disconn_success)
		{
			printf("Disconnected from server %ssuccessfully \n%s",
					T_C_GRN, T_RESET);
		} else {
			printf("%sError: could not disconnect from server, exiting... \n%s",
					T_C_RED, T_RESET);
			exit(EXIT_FAILURE);
		}
	}

    exit (EXIT_SUCCESS);

}

