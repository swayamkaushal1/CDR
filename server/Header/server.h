#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include "process.h"
#include "auth.h"
#include "CustBillProcess.h"
#include "IntopBillProcess.h"

/* ============================================================
   Constants
   ============================================================ */
#define PORT 12345
#define BACKLOG 5
#define BUFSIZE 1024

/* ============================================================
   Data Structures
   ============================================================ */

// Structure to pass client info to thread
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} ClientInfo;

// Menu states
typedef enum {
    MAIN,
    SECOND,
    BILLING,
    CUST_BILL,
    INTER_BILL
} MenuState;

/* ============================================================
   Function Declarations
   ============================================================ */

// Socket communication helpers
int sendall(int sock, const char *buf, size_t len);
int send_line(int sock, const char *s);
ssize_t recv_line(int sock, char *buf, size_t bufsize);

// Client handling
void* client_thread(void* arg);
void handle_client(int client_fd);

#endif // SERVER_H
