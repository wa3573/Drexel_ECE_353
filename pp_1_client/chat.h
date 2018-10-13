/* Header file for fifo_seqnum_server.c and fifo_seqnum_client.c 
 *
 * Source: M. Kerrisk, Linux Programming Interface
 *
 * Author: Naga Kandasamy
 * Date created: July 10, 2018
 *
 */
#ifndef _CHAT_H_
#define _CHAT_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* Well-known name for server FIFO */
#define SERVER_FIFO "/tmp/wja35_chat_sv"

/* Template for building the client FIFO */
#define CLIENT_FIFO_TEMPLATE "/tmp/wja35_chat_client.%ld"
/* Space required to hold the client FIFO pathname; 20 bytes to hold the client pid */
#define CLIENT_FIFO_NAME_LEN (sizeof (CLIENT_FIFO_TEMPLATE) + 20)

#define SEQ_LENGTH sizeof(struct request)
#define MESSAGE_LENGTH 512
#define MAX_NUM_CLIENTS 100

struct request {
    int seq_len;        /* Length of requested sequence */
    bool global; // Is message global
    bool system; // Is this a system message?
    pid_t origin_pid;          /* PID of the client */
    pid_t dest_pid; // Destination pid
    char message[MESSAGE_LENGTH]; /* Message */

};

/* Format for the response message from server ----> client */
struct response {
    int seq_num;
    bool global; // Is message global
    bool system; // Is this a system message?
    pid_t origin_pid;
    pid_t dest_pid; // Destination pid
    char message[MESSAGE_LENGTH];
};

#endif  /* _CHAT_H_ */
