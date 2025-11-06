// server.c - simple TCP menu-driven server
// Compile on Linux: gcc -o server server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12345
#define BACKLOG 5
#define BUFSIZE 1024
#define EMAIL_MAX 64
#define PASS_MAX 32
#define USER_FILE "data/user.txt"

// Simple XOR encryption/decryption key
const char encryption_key[] = "SECRETKEY123";

// Function to encrypt/decrypt string using XOR
void encrypt_decrypt(char *data) {
    int key_len = strlen(encryption_key);
    int data_len = strlen(data);
    for(int i = 0; i < data_len; i++) {
        data[i] = data[i] ^ encryption_key[i % key_len];
    }
}

// Function to validate email format
int is_valid_email(const char *email) {
    int at_count = 0;
    int dot_after_at = 0;
    int len = strlen(email);
    
    if(len >= EMAIL_MAX || len < 5) return 0;
    
    for(int i = 0; i < len; i++) {
        if(email[i] == '@') {
            at_count++;
            continue;
        }
        if(at_count == 1 && email[i] == '.') {
            dot_after_at = 1;
        }
    }
    
    return (at_count == 1 && dot_after_at == 1);
}

// Function to validate password
int is_valid_password(const char *password) {
    int len = strlen(password);
    return (len >= 6 && len < PASS_MAX);
}

// Function to save user credentials
int save_user(const char *email, const char *password) {
    FILE *file = fopen(USER_FILE, "a");
    if(!file) return 0;
    
    // Create encrypted copies
    char enc_email[EMAIL_MAX];
    char enc_pass[PASS_MAX];
    strncpy(enc_email, email, EMAIL_MAX-1);
    strncpy(enc_pass, password, PASS_MAX-1);
    enc_email[EMAIL_MAX-1] = '\0';
    enc_pass[PASS_MAX-1] = '\0';
    
    encrypt_decrypt(enc_email);
    encrypt_decrypt(enc_pass);
    
    fprintf(file, "%s|%s\n", enc_email, enc_pass);
    fclose(file);
    return 1;
}

// Verify user credentials by reading the USER_FILE, decrypting entries,
// and comparing with provided email and password.
int verify_user(const char *email, const char *password) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;

    char line[BUFSIZE];
    while (fgets(line, sizeof(line), file)) {
        // remove trailing newline
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        // split at '|'
        char *sep = strchr(line, '|');
        if (!sep) continue;
        *sep = '\0';
        char enc_email[EMAIL_MAX];
        char enc_pass[PASS_MAX];
        strncpy(enc_email, line, EMAIL_MAX - 1);
        enc_email[EMAIL_MAX - 1] = '\0';
        strncpy(enc_pass, sep + 1, PASS_MAX - 1);
        enc_pass[PASS_MAX - 1] = '\0';

        // decrypt in-place
        encrypt_decrypt(enc_email);
        encrypt_decrypt(enc_pass);

        if (strcmp(enc_email, email) == 0 && strcmp(enc_pass, password) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

static int sendall(int sock, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

// send a null-terminated string with a terminating newline
static int send_line(int sock, const char *s) {
    char tmp[BUFSIZE];
    snprintf(tmp, sizeof(tmp), "%s\n", s);
    return sendall(sock, tmp, strlen(tmp));
}

// receive one line (up to BUFSIZE-1) into buf; returns length or -1 on error/close
static ssize_t recv_line(int sock, char *buf, size_t bufsize) {
    size_t idx = 0;
    while (idx + 1 < bufsize) {
        char c;
        ssize_t r = recv(sock, &c, 1, 0);
        if (r <= 0) return -1; // closed or error
        if (c == '\n') break;
        if (c == '\r') continue;
        buf[idx++] = c;
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

void handle_client(int client_fd) {
    char buf[BUFSIZE];
    enum {MAIN, SECOND, BILLING, CUST_BILL, INTER_BILL} state = MAIN;
    int connected = 1;

    while (connected) {
        if (state == MAIN) {
            send_line(client_fd, "-- MAIN MENU --");
            send_line(client_fd, "1) Signup");
            send_line(client_fd, "2) Login");
            send_line(client_fd, "3) Exit");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {  // Signup
                // Request email
                send_line(client_fd, "Enter email:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                // Validate email
                if (!is_valid_email(buf)) {
                    send_line(client_fd, "Invalid email format. Returning to main menu.");
                    continue;
                }
                char email[EMAIL_MAX];
                strncpy(email, buf, EMAIL_MAX-1);
                email[EMAIL_MAX-1] = '\0';
                
                // Request password
                send_line(client_fd, "Enter password (minimum 6 characters):");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                // Validate password
                if (!is_valid_password(buf)) {
                    send_line(client_fd, "Invalid password. Must be at least 6 characters. Returning to main menu.");
                    continue;
                }
                
                // Save user
                if (save_user(email, buf)) {
                    send_line(client_fd, "Signup successful! Please login.");
                } else {
                    send_line(client_fd, "Error creating account. Please try again.");
                }
            } else if (strcmp(buf, "2") == 0) {  // Login
                // Request email
                send_line(client_fd, "Enter email:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;

                // Validate email
                if (!is_valid_email(buf)) {
                    send_line(client_fd, "Invalid email format. Returning to main menu.");
                    continue;
                }
                char email[EMAIL_MAX];
                strncpy(email, buf, EMAIL_MAX-1);
                email[EMAIL_MAX-1] = '\0';

                // Request password
                send_line(client_fd, "Enter password:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;

                // Verify credentials
                if (verify_user(email, buf)) {
                    send_line(client_fd, "Login successful. Welcome!");
                    state = SECOND;
                } else {
                    send_line(client_fd, "Invalid credentials. Returning to main menu.");
                }
            } else if (strcmp(buf, "3") == 0) {
                send_line(client_fd, "Goodbye. Closing connection.");
                break;
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == SECOND) {
            send_line(client_fd, "-- SECONDARY MENU --");
            send_line(client_fd, "1) Process the CDR data");
            send_line(client_fd, "2) Print and search");
            send_line(client_fd, "3) Logout");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                send_line(client_fd, "Feature is coming soon");
                // remain in SECOND menu
            } else if (strcmp(buf, "2") == 0) {
                state = BILLING;
            } else if (strcmp(buf, "3") == 0) {
                state = MAIN; // back to main menu
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == BILLING) {
            send_line(client_fd, "-- PRINT & SEARCH MENU --");
            send_line(client_fd, "1) Customer Billing");
            send_line(client_fd, "2) Interoperator Billing");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                state = CUST_BILL;
            } else if (strcmp(buf, "2") == 0) {
                state = INTER_BILL;
            } else if (strcmp(buf, "3") == 0) {
                state = SECOND;
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == CUST_BILL) {
            send_line(client_fd, "-- CUSTOMER BILLING --");
            send_line(client_fd, "1) Search by msisdn no");
            send_line(client_fd, "2) Print file content of CB.txt");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0 || strcmp(buf, "2") == 0) {
                send_line(client_fd, "Feature is coming soon. Closing connection.");
                break; // close connection per spec
            } else if (strcmp(buf, "3") == 0) {
                state = BILLING;
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == INTER_BILL) {
            send_line(client_fd, "-- INTEROP BILLING --");
            send_line(client_fd, "1) Search by operator name");
            send_line(client_fd, "2) Print file content of IOSB.txt");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0 || strcmp(buf, "2") == 0) {
                send_line(client_fd, "Feature is coming soon. Closing connection.");
                break;
            } else if (strcmp(buf, "3") == 0) {
                state = BILLING;
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        }
    }
    close(client_fd);
}

int main(void) {
    int sockfd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t sin_size = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        close(sockfd);
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        printf("Connection from %s\n", inet_ntoa(client_addr.sin_addr));
        handle_client(client_fd);
        printf("Client disconnected\n");
        // continue to accept new clients
    }

    close(sockfd);
    return 0;
}
