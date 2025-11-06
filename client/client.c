// client.c - simple TCP client for the menu-driven server
// Compile on Linux: gcc -o client client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

#define PORT 12345
#define BUFSIZE 1024

static ssize_t recv_line(int sock, char *buf, size_t bufsize) {
    size_t idx = 0;
    while (idx + 1 < bufsize) {
        char c;
        ssize_t r = recv(sock, &c, 1, 0);
        if (r == 0) return 0; // closed
        if (r < 0) return -1;
        if (c == '\n') break;
        if (c == '\r') continue;
        buf[idx++] = c;
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

int main(int argc, char **argv) {
    const char *server_ip = "127.0.0.1";
    if (argc >= 2) server_ip = argv[1];

    int sockfd;
    struct sockaddr_in serv_addr;
    char buf[BUFSIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to %s:%d\n", server_ip, PORT);

    // Read loop: server will send lines; when a prompt 'Enter choice' appears,
    // read user input and send it.
    while (1) {
        ssize_t r = recv_line(sockfd, buf, sizeof(buf));
        if (r == 0) {
            printf("Server closed connection.\n");
            break;
        } else if (r < 0) {
            perror("recv");
            break;
        }
        // print server line
        printf("%s\n", buf);
        // if the server asks for input (choice or credentials)
            if (strstr(buf, "Enter choice") != NULL || 
                strstr(buf, "Enter email") != NULL || 
                strstr(buf, "Enter password") != NULL ||
                strstr(buf, "Enter MSISDN") != NULL ||
                strstr(buf, "Enter operator name") != NULL ||
                strstr(buf, "Press Enter") != NULL) {
                // read from stdin; if server asked for password, disable echo
                char input[256];
                input[0] = '\0';
                if (strstr(buf, "Enter password") != NULL) {
                    // Read password without echo (POSIX - works with PuTTY)
                    struct termios oldt, newt;
                    if (tcgetattr(fileno(stdin), &oldt) == 0) {
                        newt = oldt;
                        newt.c_lflag &= ~(ECHO);
                        tcsetattr(fileno(stdin), TCSANOW, &newt);
                        if (fgets(input, sizeof(input), stdin) == NULL) {
                            // EOF
                            tcsetattr(fileno(stdin), TCSANOW, &oldt);
                            printf("Input closed. Disconnecting.\n");
                            break;
                        }
                        tcsetattr(fileno(stdin), TCSANOW, &oldt);
                    } else {
                        // fallback to normal fgets
                        if (fgets(input, sizeof(input), stdin) == NULL) {
                            printf("Input closed. Disconnecting.\n");
                            break;
                        }
                    }
                    // trim newline if present
                    size_t lenp = strlen(input);
                    if (lenp > 0 && input[lenp-1] == '\n') input[lenp-1] = '\0';
                    // print a newline to move prompt (since echo was off)
                    printf("\n");
                } else {
                    if (fgets(input, sizeof(input), stdin) == NULL) {
                        // EOF on stdin; close
                        printf("Input closed. Disconnecting.\n");
                        break;
                    }
                    // trim newline
                    size_t len = strlen(input);
                    if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';
                }

                // send with newline
                char out[512];
                snprintf(out, sizeof(out), "%s\n", input);
                if (send(sockfd, out, strlen(out), 0) <= 0) {
                    perror("send");
                    break;
                }
            }
    }

    close(sockfd);
    return 0;
}
