// CustomerBilling.c - Customer billing search and display functions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "../Header/CustBillProcess.h"

#define BUFSIZE 1024

// Send all helper for socket - ensures complete data transmission
static int sendall_fd(int sock, const char *buf, size_t len) {
    size_t total = 0;
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        
        if (n > 0) {
            total += n;
            retry_count = 0; // Reset retry count on success
        } else if (n == 0) {
            // Connection closed
            return -1;
        } else {
            // Error occurred
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // Temporary error - retry
                retry_count++;
                if (retry_count > MAX_RETRIES) {
                    return -1;
                }
                usleep(1000); // Wait 1ms before retry
                continue;
            }
            // Permanent error
            return -1;
        }
    }
    return 0;
}

// Send a line with newline appended
static int send_line_fd(int sock, const char *s) {
    char tmp[BUFSIZE];
    int len = snprintf(tmp, sizeof(tmp), "%s\n", s);
    if (len >= BUFSIZE) {
        // Message truncated, but still try to send
        len = BUFSIZE - 1;
    }
    return sendall_fd(sock, tmp, len);
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


void display_customer_billing_file(int client_fd, const char *filename) {
    FILE *file = fopen(filename, "r");
    char line[2048];
    int line_count = 0;

    if (!file) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Error opening file: %s", strerror(errno));
        send_line_fd(client_fd, msg);
        snprintf(msg, sizeof(msg), "Note: Please process the CDR data first using option 1 from the main menu.");
        send_line_fd(client_fd, msg);
        return;
    }

    send_line_fd(client_fd, "=== Customer Billing File Content ===");

    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline/carriage return
        line[strcspn(line, "\r\n")] = 0;
        
        // Send line using send_line_fd (which adds \n)
        if (send_line_fd(client_fd, line) != 0) {
            // Client disconnected
            fclose(file);
            return;
        }
        
        line_count++;
        // Add a small delay every 10 lines to prevent socket buffer overflow
        if (line_count % 10 == 0) {
            usleep(10000); // 10ms delay
        }
    }

    send_line_fd(client_fd, "=== End of File ===");
    fclose(file);
    
    // Now send the file transfer marker and transfer the file
    send_line_fd(client_fd, "FILE_TRANSFER_START:CB.txt");
    
    // Reopen file for binary transfer
    file = fopen(filename, "rb");
    if (!file) {
        send_line_fd(client_fd, "FILE_TRANSFER_ERROR");
        return;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    
    // Send file size
    char size_msg[64];
    snprintf(size_msg, sizeof(size_msg), "FILE_SIZE:%ld", filesize);
    send_line_fd(client_fd, size_msg);
    
    // Send file data in chunks
    char buffer[8192];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (sendall_fd(client_fd, buffer, bytes_read) != 0) {
            fclose(file);
            return;
        }
    }
    
    fclose(file);
    send_line_fd(client_fd, "FILE_TRANSFER_COMPLETE");
}