// process.c - CDR processing coordinator
// Spawns parallel threads for customer and interoperator billing processing

#include "../Header/process.h"

/* ============================================================
   Socket Communication Helpers
   ============================================================ */

int sendall_fd(int sock, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

int send_line_fd(int sock, const char *s) {
    char tmp[BUFSIZE];
    snprintf(tmp, sizeof(tmp), "%s\n", s);
    return sendall_fd(sock, tmp, strlen(tmp));
}

/* ============================================================
   CDR Processing Coordinator
   ============================================================ */

int processCDRdata(int client_fd, const char *output_dir) {
    pthread_t t1, t2;
    int rc;
    
    // Allocate thread arguments
    ProcessThreadArg *arg = (ProcessThreadArg *)malloc(sizeof(ProcessThreadArg));
    if (!arg) {
        send_line_fd(client_fd, "Error: memory allocation failed");
        return 0;
    }
    strncpy(arg->output_dir, output_dir, sizeof(arg->output_dir) - 1);
    arg->output_dir[sizeof(arg->output_dir) - 1] = '\0';

    // Inform client that processing has started
    send_line_fd(client_fd, "Processing CDR data: started...");

    rc = pthread_create(&t1, NULL, custbillprocess, arg);
    if (rc != 0) {
        send_line_fd(client_fd, "Error: failed to start Customer Billing processing thread");
        free(arg);
        return 0;
    }

    rc = pthread_create(&t2, NULL, intopbillprocess, arg);
    if (rc != 0) {
        send_line_fd(client_fd, "Error: failed to start Interoperator Billing processing thread");
        // join thread 1 if needed
        pthread_join(t1, NULL);
        free(arg);
        return 0;
    }

    // Optionally, while waiting, we could stream progress updates. For now just join.
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    // Free allocated argument
    free(arg);

    // Both parts done
    send_line_fd(client_fd, "Processing CDR data: completed.");
    return 1;
}
