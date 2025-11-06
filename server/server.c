/*
 * ----------------------------------------------------------------------------
 *  File Name   : server.c
 *  Author      : Swayam Kaushal, Soham Bose
 *  Created On  : November 6, 2025
 *  Version     : 1.0
 *
 *  Description : Multi-threaded TCP server for telecom billing system.
 *                Handles client connections and provides a menu-driven interface
 *                for user authentication, CDR processing, and billing operations.
 *                Supports customer and inter-operator billing functionalities.
 *
 *  Features    :
 *    - Signup/Login with email and password validation
 *    - CDR data processing with concurrent worker threads
 *    - Search and display billing records (CB.txt, IOSB.txt)
 *    - Graceful client disconnection after each operation
 *    - Logging of all major events (auth, menu choices, file operations)
 *
 *  Compilation : gcc -o server server.c Auth/auth.c Log/Log.c Process/process.c \
 *                Process/CustBillProcess.c Process/IntopBillProcess.c \
 *                Billing/CustomerBilling.c Billing/InteroperatorBilling.c -lpthread
 *
 *  License     : © 2025 Swayam Kaushal, Soham Bose. All rights reserved.
 *                This source code is intended for internal use only.
 *                Unauthorized redistribution or modification is prohibited.
 *
 *  Notes       :
 *    - Ensure required modules and header files are present before compilation.
 *    - Server listens on defined PORT and handles multiple clients via threads.
 *    - SIGPIPE is ignored to prevent crashes on client disconnection.
 * ----------------------------------------------------------------------------
 */
#include "Header/server.h"

/* ============================================================
   Socket Communication Helpers
   ============================================================ */

int sendall(int sock, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

int send_line(int sock, const char *s) {
    char tmp[BUFSIZE];
    snprintf(tmp, sizeof(tmp), "%s\n", s);
    return sendall(sock, tmp, strlen(tmp));
}

ssize_t recv_line(int sock, char *buf, size_t bufsize) {
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

/* ============================================================
   Client Thread Handling
   ============================================================ */

void* client_thread(void* arg) {
    ClientInfo *info = (ClientInfo *)arg;
    int client_fd = info->client_fd;
    
    printf("Thread started for client %s\n", inet_ntoa(info->client_addr.sin_addr));
    
    // Free the allocated ClientInfo structure
    free(info);
    
    // Handle the client
    handle_client(client_fd);
    
    printf("Thread ending, client disconnected\n");
    
    return NULL;
}

/* ============================================================
   Client Request Handler (State Machine)
   ============================================================ */

void handle_client(int client_fd) {
    char buf[BUFSIZE];
    MenuState state = MAIN;
    int connected = 1;
    char logged_in_user[EMAIL_MAX] = {0}; // Track logged-in user email
    char user_output_dir[256] = {0}; // User-specific output directory

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
                send_line(client_fd, "Enter password (min 6 chars, must include: uppercase, lowercase, digit, special char):");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                // Validate password (strong validation from auth module)
                if (!is_valid_password(buf)) {
                    send_line(client_fd, "Invalid password. Must be at least 6 characters with uppercase, lowercase, digit, and special character. Returning to main menu.");
                    continue;
                }
                
                // Save user (auth module checks for duplicates)
                int result = save_user(email, buf);
                if (result == 1) {
                    send_line(client_fd, "Signup successful! Please login.");
                } else if (result == -1) {
                    send_line(client_fd, "Email already registered. Please login or use a different email.");
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
                    // Store logged-in user email
                    strncpy(logged_in_user, email, EMAIL_MAX-1);
                    logged_in_user[EMAIL_MAX-1] = '\0';
                    
                    // Create user-specific output directory: Output/<sanitized_email>/
                    char sanitized[EMAIL_MAX];
                    strncpy(sanitized, email, EMAIL_MAX-1);
                    sanitized[EMAIL_MAX-1] = '\0';
                    // Replace @ and . with _ for safe directory name
                    for (int i = 0; sanitized[i]; i++) {
                        if (sanitized[i] == '@' || sanitized[i] == '.') {
                            sanitized[i] = '_';
                        }
                    }
                    snprintf(user_output_dir, sizeof(user_output_dir), "Output/%s", sanitized);
                    
                    // Create the directory (mkdir returns 0 on success, -1 if exists or error)
                    mkdir(user_output_dir, 0755);
                    
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
                // Process the CDR data: run two worker functions concurrently
                // processCDRdata will send progress/completion messages to client
                processCDRdata(client_fd, user_output_dir);
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
            send_line(client_fd, "4) Exit");
            send_line(client_fd, "Enter choice (1-4):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                // Search by MSISDN
                send_line(client_fd, "Enter MSISDN to search:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                long msisdn = atol(buf);
                if (msisdn <= 0) {
                    send_line(client_fd, "Invalid MSISDN. Please enter a valid number.");
                } else {
                    // Use user-specific CB.txt file
                    char cb_path[300];
                    snprintf(cb_path, sizeof(cb_path), "%s/CB.txt", user_output_dir);
                    search_msisdn(client_fd, cb_path, msisdn);
                }
                // After search, go back to SECOND menu
                state = SECOND;
            } else if (strcmp(buf, "2") == 0) {
                // Display CB.txt content
                char cb_path[300];
                snprintf(cb_path, sizeof(cb_path), "%s/CB.txt", user_output_dir);
                display_customer_billing_file(client_fd, cb_path);
                // After displaying, go back to SECOND menu
                state = SECOND;
            } else if (strcmp(buf, "3") == 0) {
                state = BILLING;
            } else if (strcmp(buf, "4") == 0) {
                send_line(client_fd, "Goodbye. Closing connection.");
                connected = 0; // disconnect client, server continues
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == INTER_BILL) {
            send_line(client_fd, "-- INTEROP BILLING --");
            send_line(client_fd, "1) Search by operator name");
            send_line(client_fd, "2) Print file content of IOSB.txt");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "4) Exit");
            send_line(client_fd, "Enter choice (1-4):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                // Search by operator name
                send_line(client_fd, "Enter operator name to search:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                if (strlen(buf) == 0) {
                    send_line(client_fd, "Invalid operator name. Please enter a valid name.");
                } else {
                    // Use user-specific IOSB.txt file
                    char iosb_path[300];
                    snprintf(iosb_path, sizeof(iosb_path), "%s/IOSB.txt", user_output_dir);
                    search_operator(client_fd, iosb_path, buf);
                }
                // After search, go back to SECOND menu
                state = SECOND;
            } else if (strcmp(buf, "2") == 0) {
                // Display IOSB.txt content
                char iosb_path[300];
                snprintf(iosb_path, sizeof(iosb_path), "%s/IOSB.txt", user_output_dir);
                display_interoperator_billing_file(client_fd, iosb_path);
                // After displaying, go back to SECOND menu
                state = SECOND;
            } else if (strcmp(buf, "3") == 0) {
                state = BILLING;
            } else if (strcmp(buf, "4") == 0) {
                send_line(client_fd, "Goodbye. Closing connection.");
                connected = 0; // disconnect client, server continues
            } else {
                send_line(client_fd, "Invalid choice. Try again.");
            }
        }
    }
    close(client_fd);
}

/* ============================================================
   Main Server Loop
   ============================================================ */

int main(void) {
    int sockfd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t sin_size = sizeof(client_addr);

    // Ignore SIGPIPE signal - prevents server crash when client disconnects during write
    signal(SIGPIPE, SIG_IGN);

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
        
        // Allocate memory for client info
        ClientInfo *info = (ClientInfo *)malloc(sizeof(ClientInfo));
        if (!info) {
            fprintf(stderr, "Failed to allocate memory for client info\n");
            close(client_fd);
            continue;
        }
        info->client_fd = client_fd;
        info->client_addr = client_addr;
        
        // Create a new thread for this client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, info) != 0) {
            fprintf(stderr, "Failed to create thread for client\n");
            free(info);
            close(client_fd);
            continue;
        }
        
        // Detach the thread so it cleans up automatically when done
        pthread_detach(thread_id);
        
        // Continue accepting new clients immediately
    }

    close(sockfd);
    return 0;
}
