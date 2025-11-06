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
    char client_ip[INET_ADDRSTRLEN];
    
    // Store client IP for logging
    strncpy(client_ip, inet_ntoa(info->client_addr.sin_addr), INET_ADDRSTRLEN-1);
    client_ip[INET_ADDRSTRLEN-1] = '\0';
    
    printf("Thread started for client %s\n", client_ip);
    log_connection_event(client_ip, "Thread Started");
    
    // Free the allocated ClientInfo structure
    free(info);
    
    // Handle the client
    handle_client(client_fd);
    
    printf("Thread ending, client disconnected\n");
    log_connection_event(client_ip, "Thread Ended - Client Disconnected");
    
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
    int cdr_processed = 0; // Track if CDR has been processed (0 = not processed, 1 = processed)

    LOG_DEBUG("handle_client: Starting client handler");
    
    while (connected) {
        if (state == MAIN) {
            send_line(client_fd, "-- MAIN MENU --");
            send_line(client_fd, "1) Signup");
            send_line(client_fd, "2) Login");
            send_line(client_fd, "3) Exit");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {  // Signup
                log_menu_choice("GUEST", "MAIN MENU", "Signup");
                
                // Request email
                send_line(client_fd, "Enter email:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                // Validate email
                if (!is_valid_email(buf)) {
                    LOG_WARN("Signup failed: Invalid email format");
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
                    LOG_WARN("Signup failed: Invalid password format for user: %s", email);
                    send_line(client_fd, "Invalid password. Must be at least 6 characters with uppercase, lowercase, digit, and special character. Returning to main menu.");
                    continue;
                }
                
                // Save user (auth module checks for duplicates)
                int result = save_user(email, buf);
                if (result == 1) {
                    log_auth_event(email, "Signup", 1);
                    send_line(client_fd, "Signup successful! Please login.");
                } else if (result == -1) {
                    log_auth_event(email, "Signup - Duplicate", 0);
                    send_line(client_fd, "Email already registered. Please login or use a different email.");
                } else {
                    log_auth_event(email, "Signup - Error", 0);
                    send_line(client_fd, "Error creating account. Please try again.");
                }
            } else if (strcmp(buf, "2") == 0) {  // Login
                log_menu_choice("GUEST", "MAIN MENU", "Login");
                
                // Request email
                send_line(client_fd, "Enter email:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;

                // Validate email
                if (!is_valid_email(buf)) {
                    LOG_WARN("Login failed: Invalid email format");
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
                    
                    log_auth_event(email, "Login", 1);
                    send_line(client_fd, "Login successful. Welcome!");
                    state = SECOND;
                } else {
                    log_auth_event(email, "Login", 0);
                    send_line(client_fd, "Invalid credentials. Returning to main menu.");
                }
            } else if (strcmp(buf, "3") == 0) {
                log_menu_choice("GUEST", "MAIN MENU", "Exit");
                LOG_INFO("Client requested exit from main menu");
                send_line(client_fd, "Goodbye. Closing connection.");
                break;
            } else {
                LOG_DEBUG("Invalid main menu choice: %s", buf);
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
                log_menu_choice(logged_in_user, "SECONDARY MENU", "Process CDR Data");
                log_processing_event(logged_in_user, "CDR Processing", "Started");
                
                // Process the CDR data: run two worker functions concurrently
                // processCDRdata will send progress/completion messages to client
                processCDRdata(client_fd, user_output_dir);
                
                log_processing_event(logged_in_user, "CDR Processing", "Completed");
                cdr_processed = 1; // Mark CDR as processed
                // remain in SECOND menu
            } else if (strcmp(buf, "2") == 0) {
                log_menu_choice(logged_in_user, "SECONDARY MENU", "Print and Search");
                
                // Check if CDR has been processed
                if (cdr_processed == 0) {
                    LOG_WARN("User %s attempted to access billing without processing CDR", logged_in_user);
                    send_line(client_fd, "ERROR: Please process the CDR data first (Option 1) before accessing billing.");
                    // Stay in SECOND menu
                } else {
                    state = BILLING;
                }
            } else if (strcmp(buf, "3") == 0) {
                log_menu_choice(logged_in_user, "SECONDARY MENU", "Logout");
                log_auth_event(logged_in_user, "Logout", 1);
                memset(logged_in_user, 0, EMAIL_MAX);
                cdr_processed = 0; // Reset CDR processed flag on logout
                state = MAIN; // back to main menu
            } else {
                LOG_DEBUG("Invalid secondary menu choice: %s", buf);
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
                log_menu_choice(logged_in_user, "BILLING MENU", "Customer Billing");
                state = CUST_BILL;
            } else if (strcmp(buf, "2") == 0) {
                log_menu_choice(logged_in_user, "BILLING MENU", "Interoperator Billing");
                state = INTER_BILL;
            } else if (strcmp(buf, "3") == 0) {
                log_menu_choice(logged_in_user, "BILLING MENU", "Back");
                state = SECOND;
            } else {
                LOG_DEBUG("Invalid billing menu choice: %s", buf);
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == CUST_BILL) {
            send_line(client_fd, "-- CUSTOMER BILLING --");
            send_line(client_fd, "1) Search by msisdn no");
            send_line(client_fd, "2) Print file content of CB.txt");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                log_menu_choice(logged_in_user, "CUSTOMER BILLING", "Search by MSISDN");
                
                // Search by MSISDN
                send_line(client_fd, "Enter MSISDN to search:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                long msisdn = atol(buf);
                if (msisdn <= 0) {
                    LOG_WARN("Invalid MSISDN entered: %s", buf);
                    send_line(client_fd, "Invalid MSISDN. Please enter a valid number.");
                } else {
                    // Use user-specific CB.txt file
                    char cb_path[300];
                    snprintf(cb_path, sizeof(cb_path), "%s/CB.txt", user_output_dir);
                    
                    char search_val[32];
                    snprintf(search_val, sizeof(search_val), "%ld", msisdn);
                    
                    // Perform search (assuming it returns success/failure)
                    search_msisdn(client_fd, cb_path, msisdn);
                    log_search_event(logged_in_user, "MSISDN", search_val, 1);
                }
                // After search, disconnect client as per requirement
                log_file_operation(logged_in_user, "CB.txt", "Search Completed");
                send_line(client_fd, "Operation completed. Disconnecting...");
                connected = 0; // disconnect client, server continues
            } else if (strcmp(buf, "2") == 0) {
                log_menu_choice(logged_in_user, "CUSTOMER BILLING", "Print CB.txt");
                
                // Display CB.txt content
                char cb_path[300];
                snprintf(cb_path, sizeof(cb_path), "%s/CB.txt", user_output_dir);
                display_customer_billing_file(client_fd, cb_path);
                
                log_file_operation(logged_in_user, "CB.txt", "File Sent to Client");
                // After displaying, disconnect client as per requirement
                send_line(client_fd, "Operation completed. Disconnecting...");
                connected = 0; // disconnect client, server continues
            } else if (strcmp(buf, "3") == 0) {
                log_menu_choice(logged_in_user, "CUSTOMER BILLING", "Back");
                state = BILLING;
            } else {
                LOG_DEBUG("Invalid customer billing choice: %s", buf);
                send_line(client_fd, "Invalid choice. Try again.");
            }
        } else if (state == INTER_BILL) {
            send_line(client_fd, "-- INTEROP BILLING --");
            send_line(client_fd, "1) Search by operator name");
            send_line(client_fd, "2) Print file content of IOSB.txt");
            send_line(client_fd, "3) Back");
            send_line(client_fd, "Enter choice (1-3):");
            if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
            if (strcmp(buf, "1") == 0) {
                log_menu_choice(logged_in_user, "INTEROP BILLING", "Search by Operator");
                
                // Search by operator name
                send_line(client_fd, "Enter operator name to search:");
                if (recv_line(client_fd, buf, sizeof(buf)) <= 0) break;
                
                if (strlen(buf) == 0) {
                    LOG_WARN("Invalid operator name entered (empty)");
                    send_line(client_fd, "Invalid operator name. Please enter a valid name.");
                } else {
                    // Use user-specific IOSB.txt file
                    char iosb_path[300];
                    snprintf(iosb_path, sizeof(iosb_path), "%s/IOSB.txt", user_output_dir);
                    search_operator(client_fd, iosb_path, buf);
                    
                    log_search_event(logged_in_user, "Operator", buf, 1);
                }
                // After search, disconnect client as per requirement
                log_file_operation(logged_in_user, "IOSB.txt", "Search Completed");
                send_line(client_fd, "Operation completed. Disconnecting...");
                connected = 0; // disconnect client, server continues
            } else if (strcmp(buf, "2") == 0) {
                log_menu_choice(logged_in_user, "INTEROP BILLING", "Print IOSB.txt");
                
                // Display IOSB.txt content
                char iosb_path[300];
                snprintf(iosb_path, sizeof(iosb_path), "%s/IOSB.txt", user_output_dir);
                display_interoperator_billing_file(client_fd, iosb_path);
                
                log_file_operation(logged_in_user, "IOSB.txt", "File Sent to Client");
                // After displaying, disconnect client as per requirement
                send_line(client_fd, "Operation completed. Disconnecting...");
                connected = 0; // disconnect client, server continues
            } else if (strcmp(buf, "3") == 0) {
                log_menu_choice(logged_in_user, "INTEROP BILLING", "Back");
                state = BILLING;
            } else {
                LOG_DEBUG("Invalid interop billing choice: %s", buf);
                send_line(client_fd, "Invalid choice. Try again.");
            }
        }
    }
    
    LOG_INFO("Closing client connection");
    close(client_fd);
}

/* ============================================================
   Main Server Loop
   ============================================================ */

int main(void) {
    int sockfd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t sin_size = sizeof(client_addr);

    // Initialize logging system
    if (log_init("ServerLog/server.log", LOG_DEBUG, 1) != 0) {
        fprintf(stderr, "Failed to initialize logging system\n");
        return 1;
    }
    
    LOG_INFO("=== CDR Server Starting ===");
    LOG_INFO("Server version: 1.0");
    
    // Ignore SIGPIPE signal - prevents server crash when client disconnects during write
    signal(SIGPIPE, SIG_IGN);
    LOG_DEBUG("SIGPIPE signal handler configured");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOG_FATAL("Failed to create socket: %s", strerror(errno));
        perror("socket");
        log_cleanup();
        return 1;
    }
    LOG_DEBUG("Socket created successfully");

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_DEBUG("Socket options configured (SO_REUSEADDR)");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        LOG_FATAL("Failed to bind socket to port %d: %s", PORT, strerror(errno));
        perror("bind");
        close(sockfd);
        log_cleanup();
        return 1;
    }
    LOG_INFO("Socket bound to port %d", PORT);

    if (listen(sockfd, BACKLOG) == -1) {
        LOG_FATAL("Failed to listen on socket: %s", strerror(errno));
        perror("listen");
        close(sockfd);
        log_cleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);
    LOG_INFO("Server listening on port %d (backlog: %d)", PORT, BACKLOG);

    while (1) {
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            LOG_WARN("Failed to accept connection: %s", strerror(errno));
            perror("accept");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        strncpy(client_ip, inet_ntoa(client_addr.sin_addr), INET_ADDRSTRLEN-1);
        client_ip[INET_ADDRSTRLEN-1] = '\0';
        
        printf("Connection from %s\n", client_ip);
        log_connection_event(client_ip, "Connected");
        
        // Allocate memory for client info
        ClientInfo *info = (ClientInfo *)malloc(sizeof(ClientInfo));
        if (!info) {
            LOG_FATAL("Failed to allocate memory for client info");
            fprintf(stderr, "Failed to allocate memory for client info\n");
            close(client_fd);
            log_connection_event(client_ip, "Rejected - Memory allocation failed");
            continue;
        }
        info->client_fd = client_fd;
        info->client_addr = client_addr;
        
        // Create a new thread for this client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, info) != 0) {
            LOG_FATAL("Failed to create thread for client %s", client_ip);
            fprintf(stderr, "Failed to create thread for client\n");
            free(info);
            close(client_fd);
            log_connection_event(client_ip, "Rejected - Thread creation failed");
            continue;
        }
        
        LOG_INFO("Thread created for client %s (Thread ID: %lu)", client_ip, (unsigned long)thread_id);
        
        // Detach the thread so it cleans up automatically when done
        pthread_detach(thread_id);
        
        // Continue accepting new clients immediately
    }

    LOG_INFO("Server shutting down");
    close(sockfd);
    log_cleanup();
    return 0;
}
