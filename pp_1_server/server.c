/*
 * 	Chat Room: Server
 *
 * 	adapted from fifo_seqnum_server.c
 *
 *  Created on: Jul 11, 2018
 *      Author: William Anderson
 */

#include <signal.h>

#include "chat.h"
#include "list.h"
#include "style.h"

#define MAX_NUM_CLIENTS 100

int compare_pidt(const void* lhs, const void* rhs);
void free_pidt(void* data);
bool send_sys_response(struct response* resp, struct request* req);
bool client_remove (list* client_pids_list, pid_t* client_pid);
void resp_format_OK(struct response* resp, pid_t dest, int seq_num);
void resp_format_FAIL(struct response* resp, pid_t dest, int seq_num);
void resp_format_msg(struct response* resp, struct request* req, int seq_num);
void req_reset(struct request* req);

void free_pidt(void* data)
{
	free((pid_t*)data);
}

int compare_pidt(const void* lhs, const void* rhs)
{
	const pid_t* llhs = (const pid_t*)lhs;
	const pid_t* lrhs = (const pid_t*)rhs;

	if (*llhs < *lrhs)
	{
		return -1;
	} else if (*llhs > *lrhs) {
		return 1;
	}

	return 0;
}

bool send_sys_response(struct response* resp, struct request* req)
{
	int client_fd;
	char client_fifo[CLIENT_FIFO_NAME_LEN];

	snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)req->origin_pid);
    // printf ("Opening client FIFO %s \n", client_fifo);

    client_fd = open (client_fifo, O_WRONLY);
    if (client_fd == -1){    /* Open failed on the client FIFO. Give up and move on. */
        printf ("Error opening client fifo %s \n", client_fifo);
        return false;
    }

    if (write (client_fd, resp, sizeof (struct response)) != sizeof (struct response))
    {
    	fprintf (stderr, "Error writing to client FIFO %s \n", client_fifo);
    	return false;
    }

    if (close (client_fd) == -1)
    {
    	fprintf (stderr, "Error closing client FIFO %s \n", client_fifo);
    	return false;
    }

    return true;
}

bool send_msg(struct response* resp, struct request* req)
{
	int client_fd;
	char client_fifo[CLIENT_FIFO_NAME_LEN];

	snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)req->dest_pid);
    // printf ("Opening client FIFO %s \n", client_fifo);

    client_fd = open (client_fifo, O_WRONLY);
    if (client_fd == -1){    /* Open failed on the client FIFO. Give up and move on. */
        printf ("Error opening client fifo %s \n", client_fifo);
        return false;
    }

    if (write (client_fd, resp, sizeof (struct response)) != sizeof (struct response))
    {
    	fprintf (stderr, "Error writing to client FIFO %s \n", client_fifo);
    	return false;
    }

    if (close (client_fd) == -1)
    {
    	fprintf (stderr, "Error closing client FIFO %s \n", client_fifo);
    	return false;
    }

    return true;
}

bool client_remove (list* client_pids_list, pid_t* client_pid)
{
	   bool client_stored = list_contains(client_pids_list, client_pid);

    if (client_stored)
    {
 	   list_remove_pid(client_pids_list, client_pid);
 	   return true;
    }

    return false;
}

void resp_format_OK(struct response* resp, pid_t dest, int seq_num)
{
    resp->dest_pid = dest;
    resp->global = false;
    resp->system = true;
    strcpy(resp->message, "OK");
}

void resp_format_FAIL(struct response* resp, pid_t dest, int seq_num)
{
    resp->dest_pid = dest;
    resp->global = false;
    resp->system = true;
    strcpy(resp->message, "FAIL");
}

void resp_format_msg(struct response* resp, struct request* req, int seq_num)
{
	char buffer[512];
	resp->seq_num = seq_num;

    sprintf(buffer, "%s", req->message);
    strcpy(resp->message, buffer);

    resp->origin_pid = req->origin_pid;
    resp->dest_pid = req->dest_pid;
    resp->global = req->global;
}

void req_reset(struct request* req)
{
    req->system = false;
    req->global = false;
//    memset(req->message, 0, (size_t)MESSAGE_LENGTH); // Probably unnecessary
}


int main (int argc, char **argv)
{
    /* Create a well-known FIFO and open it for reading. The server
     * must be run before any of its clients so that the server FIFO exists by the
     * time a client attempts to open it. The server's open() blocks until the first client
     * opens the other end of the server FIFO for writing.
     * */

	int client_fd;
	char client_fifo[CLIENT_FIFO_NAME_LEN];
	struct request req;
	struct response resp;
	int seq_num = 0; /* This is the service that we provide as a server */
	char buffer[512];
	bool msg_success = false;
	bool success = false;


	pid_t* client_pid;
	list* client_pids_list;

	client_pids_list = (list*)malloc(sizeof(list));
	/* Initialize list with associated functions for freeing and comparison */
	client_pids_list = list_init(client_pids_list, sizeof(pid_t), free_pidt, compare_pidt);

	printf(T_BG_C_BLU T_C_YEL "\n\t ========== Chat Room 0.1: Server ========== \n\n" T_RESET);

    umask (0);
    if (mkfifo (SERVER_FIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1 \
            && errno != EEXIST){
        perror ("mkfifo");
        exit (EXIT_FAILURE);
    }
   int server_fd = open (SERVER_FIFO, O_RDONLY);
   if (server_fd == -1){
       perror ("open");
       exit (EXIT_FAILURE);
   }

   int dummy_fd = open (SERVER_FIFO, O_WRONLY);
   if (dummy_fd == -1){
       perror ("open");
       exit (EXIT_FAILURE);
   }

   if (signal (SIGPIPE, SIG_IGN) == SIG_ERR){
       perror ("signal");
       exit (EXIT_FAILURE);
   }


   while (1){
       if (read (server_fd, &req, sizeof (struct request)) != sizeof (struct request)){
           fprintf (stderr, "Error reading request; discarding \n");
           continue;
       }

       if (req.system)
       {
    	   /* Connect client */
    	   if (strcmp(req.message, "new_client") == 0)
    	   {
    	       success = false;
    		   bool client_stored = list_contains(client_pids_list, &req.origin_pid);

    	       if (!client_stored && client_pids_list->logical_length < MAX_NUM_CLIENTS)
    	       {
    	    	   list_push_back(client_pids_list, &req.origin_pid);
    	    	   printf("Client connected with PID %s%ld%s \n",
    	    			   T_C_YEL, (long)req.origin_pid, T_RESET);
    	    	   success = true;
    	       } else if (!client_stored && client_pids_list->logical_length >= MAX_NUM_CLIENTS) {
    	    	   fprintf(stderr, "Warning: client list full, not saving client! \n");
    	    	   success = false;
    	       }

               /* Send the response to the client and close FIFO */
               if (success)
               {
            	   resp_format_OK(&resp, req.origin_pid, seq_num);
               } else {
            	   resp_format_FAIL(&resp, req.origin_pid, seq_num);
               }

               send_sys_response(&resp, &req);

               /* Update the sequence number */
               seq_num += req.seq_len;
               req_reset(&req);
               continue;

               /* Disconnect client */
    	   } else if (strcmp(req.message, "disconnect_client") == 0) {
    		   success = false;

    		   if (client_remove(client_pids_list, &req.origin_pid))
    		   {
    			   success = true;
    		   } else {
    			   success = false;
    		   }

               /* Send the response to the client and close FIFO */

               if (success)
               {
            	   resp_format_OK(&resp, req.origin_pid, seq_num);
            	   printf("Client disconnected with PID %s%ld%s \n",
            			   T_C_YEL, (long)req.origin_pid, T_RESET);
               } else {
            	   resp_format_FAIL(&resp, req.origin_pid, seq_num);
            	   printf("Client disconnect with PID %s%ld%s failed \n",
            			   T_C_YEL, (long)req.origin_pid, T_RESET);
               }

               send_sys_response(&resp, &req);

               /* Update the sequence number */
               seq_num += req.seq_len;
               req_reset(&req);

               /* Connection Check */
    	   } else if (strcmp(req.message, "check_connection") == 0) {
    		   /* Send response to sender confirming connection is active */

    		   resp_format_OK(&resp, req.origin_pid, seq_num);
               send_sys_response(&resp, &req);

               /* Update the sequence number */
               seq_num += req.seq_len;
               req_reset(&req);

               /* If it's something the server doens't recognize, send an ACK
                * anyway
                */
    	   } else {
    		   /* Send response to sender confirming message received */

    		   resp_format_OK(&resp, req.origin_pid, seq_num);
               send_sys_response(&resp, &req);

               /* Update the sequence number */
               seq_num += req.seq_len;
               req_reset(&req);
    	   }

    	   continue; 	// Iterate before proceeding to send message
       }

       	   /* SEND MESSAGE */

       if (req.global == true)
       {
    	   node* p = list_front(client_pids_list);
    	   msg_success = true;

    	   while(p->next != NULL)
    	   {
    		   client_pid = (pid_t*)(p->data);

    		   if (compare_pidt(client_pid, &req.origin_pid) == 0) // Don't send back to sender
    		   {
    			   p = p->next;
    			   continue;
    		   }

    		   printf("Sending global message to client PID: %s%ld%s \n",
    				   T_C_YEL, (long)(*client_pid), T_RESET);

    	       snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)(*client_pid));
    	       // printf ("Opening client FIFO %s \n", client_fifo);

    	       client_fd = open (client_fifo, O_WRONLY);
    	       if (client_fd == -1)
    	       {    /* Open failed on the client FIFO. Give up and move on. */
    	           fprintf (stderr, "Error opening client fifo %s, removing unreachable client...\n", client_fifo);
    	           client_remove(client_pids_list, client_pid); // Remove this client since it was unreachable
    	           msg_success = false;
    	           p = p->next;

    	           continue;
    	       }

    	       /* Send the message to the destination PID and close FIFO */

    	       resp_format_msg(&resp, &req, seq_num);

    	       if (write (client_fd, &resp, sizeof (struct response)) != sizeof (struct response))
    	       {
    	    	   fprintf (stderr, "Error writing to client FIFO %s \n", client_fifo);
    	    	   msg_success = false;
    	       }

    	       // printf ("Closing client FIFO %s \n", client_fifo);

    	       if (close (client_fd) == -1)
    	       {
    	           fprintf (stderr, "Error closing client FIFO %s \n", client_fifo);
    	           msg_success = false;
    	       }

    	       /* Update the sequence number */
    	       seq_num += req.seq_len;

    		   p = p->next;
    	   }

    	   /* Send response to sender confirming message sent */

	       if (msg_success)
	       {
	    	   printf("Message from PID %s%ld%s to %sGLOBAL%s sent successfully\n",
	    			   T_C_YEL, (long)req.origin_pid, T_RESET,
					   T_C_YEL, T_RESET);
	    	   resp_format_OK(&resp, req.origin_pid, seq_num);
//	    	   strcpy(resp.message, "OK");
	       } else {
	    	   printf("Message from PID %s%ld%s failed\n",
	    			   T_C_YEL, (long)req.origin_pid, T_RESET);
	    	   resp_format_FAIL(&resp, req.origin_pid, seq_num);
//	    	   strcpy(resp.message, "FAIL");
	       }

           send_sys_response(&resp, &req);

           /* Update the sequence number */
           seq_num += req.seq_len;
           req_reset(&req);
           continue;

       /* If destination is in list of clients */
       } else if (list_contains(client_pids_list, &req.dest_pid)) {
    	   msg_success = true;

           /* Send the message to the destination PID and close FIFO */
           resp_format_msg(&resp, &req, seq_num);

           msg_success = send_msg(&resp, &req);

           /* Update the sequence number */
           seq_num += req.seq_len;

    	   /* Send response to sender confirming message sent */

	       if (msg_success)
	       {
	    	   printf("Message from PID %s%ld%s to PID %s%ld%s sent successfully\n",
	    			   T_C_YEL, (long)req.origin_pid, T_RESET,
					   T_C_YEL, (long)req.dest_pid, T_RESET);
	    	   resp_format_OK(&resp, req.origin_pid, seq_num);
	       } else {
	    	   printf("Message from PID %s%ld%s to PID %s%ld%s failed\n",
	    			   T_C_YEL, (long)req.origin_pid, T_RESET,
					   T_C_YEL, (long)req.dest_pid, T_RESET);
	    	   resp_format_FAIL(&resp, req.origin_pid, seq_num);
	       }

           send_sys_response(&resp, &req);

           /* Update the sequence number */
           seq_num += req.seq_len;
           req_reset(&req);

       } else {
    	   fprintf (stderr, "dest_pid not in client PID list \n");

    	   resp_format_OK(&resp, req.origin_pid, seq_num);
           send_sys_response(&resp, &req);

           /* Update the sequence number */
           seq_num += req.seq_len;
           req_reset(&req);
           continue;
       }
   }
}

