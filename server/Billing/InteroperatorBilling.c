// InteroperatorBilling.c - Search and display functions for interoperator billing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../Header/IntopBillProcess.h"

#define MAX_LINE 1024

// Helper function to send a line to client via socket
static int send_line_fd(int fd, const char *line) {
    size_t len = strlen(line);
    ssize_t sent = send(fd, line, len, 0);
    return (sent == (ssize_t)len) ? 0 : -1;
}

// Helper function to convert a string to lowercase
static void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

void search_operator(int client_fd, const char *filename, const char *operator_input) {
    FILE *file = fopen(filename, "r");
    char line[MAX_LINE];
    int found = 0;

    if (!file) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Error opening file: %s\n", strerror(errno));
        send_line_fd(client_fd, msg);
        snprintf(msg, sizeof(msg), "Filename: %s\n", filename);
        send_line_fd(client_fd, msg);
        send_line_fd(client_fd, "Note: Please process the CDR data first using option 1 from the main menu.\n");
        return;
    }

    // Convert user input to lowercase
    char operator_lower[100];
    strncpy(operator_lower, operator_input, sizeof(operator_lower) - 1);
    operator_lower[sizeof(operator_lower) - 1] = '\0';
    to_lowercase(operator_lower);

    while (fgets(line, sizeof(line), file)) {
        // Make a lowercase copy of the line
        char line_lower[MAX_LINE];
        strncpy(line_lower, line, sizeof(line_lower) - 1);
        line_lower[sizeof(line_lower) - 1] = '\0';
        to_lowercase(line_lower);

        // Check if this line contains "Operator Brand:" and the searched operator
        if (strstr(line_lower, "operator brand:") && strstr(line_lower, operator_lower)) {
            found = 1;
            // Send this line (Operator Brand line)
            if (send_line_fd(client_fd, line) < 0) {
                fclose(file);
                return;
            }
            
            // Send the next 6 lines (operator details)
            for (int i = 0; i < 6; i++) {
                if (fgets(line, sizeof(line), file)) {
                    if (send_line_fd(client_fd, line) < 0) {
                        fclose(file);
                        return;
                    }
                }
            }
            break;
        }
    }

    if (!found) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Operator '%s' not found.\n", operator_input);
        send_line_fd(client_fd, msg);
    }

    fclose(file);
}

// Helper for sending all bytes
static int sendall_fd(int sock, const char *buf, size_t len) {
    size_t total = 0;
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        
        if (n > 0) {
            total += n;
            retry_count = 0;
        } else if (n == 0) {
            return -1;
        } else {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                if (retry_count > MAX_RETRIES) {
                    return -1;
                }
                usleep(1000);
                continue;
            }
            return -1;
        }
    }
    return 0;
}

void display_interoperator_billing_file(int client_fd, const char *filename) {
    FILE *file = fopen(filename, "r");
    char line[MAX_LINE];
    int line_count = 0;

    if (!file) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Error opening file: %s\n", strerror(errno));
        send_line_fd(client_fd, msg);
        snprintf(msg, sizeof(msg), "Filename: %s\n", filename);
        send_line_fd(client_fd, msg);
        send_line_fd(client_fd, "Note: Please process the CDR data first using option 1 from the main menu.\n");
        return;
    }

    send_line_fd(client_fd, "=== Interoperator Billing File Content ===\n");

    while (fgets(line, sizeof(line), file)) {
        if (send_line_fd(client_fd, line) < 0) {
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

    send_line_fd(client_fd, "=== End of File ===\n");
    fclose(file);
    
    // Now send the file transfer marker and transfer the file
    send_line_fd(client_fd, "FILE_TRANSFER_START:IOSB.txt\n");
    
    // Reopen file for binary transfer
    file = fopen(filename, "rb");
    if (!file) {
        send_line_fd(client_fd, "FILE_TRANSFER_ERROR\n");
        return;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    
    // Send file size
    char size_msg[64];
    snprintf(size_msg, sizeof(size_msg), "FILE_SIZE:%ld\n", filesize);
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
    send_line_fd(client_fd, "FILE_TRANSFER_COMPLETE\n");
}
