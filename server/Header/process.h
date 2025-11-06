#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "CustBillProcess.h"
#include "IntopBillProcess.h"

/* ============================================================
   Constants
   ============================================================ */
#define BUFSIZE 1024

/* ============================================================
   Function Declarations
   ============================================================ */

// Socket communication helpers
int sendall_fd(int sock, const char *buf, size_t len);
int send_line_fd(int sock, const char *s);

// Main CDR processing function
int processCDRdata(int client_fd, const char *output_dir);

#endif // PROCESS_H
