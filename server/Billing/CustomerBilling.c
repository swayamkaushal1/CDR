// CustomerBilling.c - Customer billing search and display functions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../Header/CustBillProcess.h"

#define BUFSIZE 1024

// Send all helper for socket
static int sendall_fd(int sock, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

// Send a line with newline appended
static int send_line_fd(int sock, const char *s) {
    char tmp[BUFSIZE];
    snprintf(tmp, sizeof(tmp), "%s\n", s);
    return sendall_fd(sock, tmp, strlen(tmp));
}

// Search for a customer by MSISDN and send results to client
void search_msisdn(int client_fd, const char *filename, long msisdn) {
    FILE *file = fopen(filename, "r");
    char line[1024];
    int found = 0;
    
    if (!file) {
        char errMsg[256];
        snprintf(errMsg, sizeof(errMsg), "Error opening file: %s", strerror(errno));
        send_line_fd(client_fd, errMsg);
        send_line_fd(client_fd, "Note: Please process the CDR data first (option 1 from secondary menu).");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        // Look for line starting with "Customer ID: "
        if (strstr(line, "Customer ID: ") != NULL) {
            long current_msisdn;
            if (sscanf(line, "Customer ID: %ld", &current_msisdn) == 1) {
                if (current_msisdn == msisdn) {
                    found = 1;
                    // Send this line and next 11 lines for complete customer info
                    line[strcspn(line, "\r\n")] = 0; // remove newline
                    send_line_fd(client_fd, line);
                    
                    for (int i = 0; i < 11; i++) {
                        if (fgets(line, sizeof(line), file)) {
                            line[strcspn(line, "\r\n")] = 0;
                            send_line_fd(client_fd, line);
                        }
                    }
                    break;
                }
            }
        }
    }

    if (!found) {
        char notFoundMsg[256];
        snprintf(notFoundMsg, sizeof(notFoundMsg), "Customer with MSISDN %ld not found.", msisdn);
        send_line_fd(client_fd, notFoundMsg);
    }

    fclose(file);
}

// Display entire customer billing file content to client
void display_customer_billing_file(int client_fd, const char *filename) {
    FILE *file = fopen(filename, "r");
    
    if (!file) {
        char errMsg[256];
        snprintf(errMsg, sizeof(errMsg), "Error opening file: %s\n", strerror(errno));
        send(client_fd, errMsg, strlen(errMsg), 0);
        snprintf(errMsg, sizeof(errMsg), "Filename: %s\n", filename);
        send(client_fd, errMsg, strlen(errMsg), 0);
        snprintf(errMsg, sizeof(errMsg), "Note: Please process the CDR data first (option 1 from secondary menu).\n");
        send(client_fd, errMsg, strlen(errMsg), 0);
        return;
    }

    char header[] = "=== Customer Billing File Content ===\n";
    send(client_fd, header, strlen(header), 0);
    
    // Read and send file contents line by line
    char line[1024];
    int lineCount = 0;
    while (fgets(line, sizeof(line), file)) {
        // fgets keeps the newline, so send as-is
        ssize_t sent = send(client_fd, line, strlen(line), 0);
        if (sent <= 0) {
            // Send failed, client may have disconnected
            fclose(file);
            return;
        }
        lineCount++;
        
        // Add small delay every line to ensure client can keep up
        usleep(1000); // 1ms delay per line
    }
    
    char footer[] = "=== End of File ===\n";
    send(client_fd, footer, strlen(footer), 0);
    fclose(file);
}
