#ifndef INTOPBILLPROCESS_H
#define INTOPBILLPROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* ============================================================
   Constants
   ============================================================ */
#define NUM_BUCKETS 4096

/* ============================================================
   Data Structures
   ============================================================ */

typedef struct OperatorStats
{
    char *operator_name;     // first seen operator name, this value is also unique.
    long total_moc_duration; // Mobile Originated Call duration (Outgoing)
    long total_mtc_duration; // Mobile Terminated Call duration (Incoming)
    long sms_mo_count;       // SMS Mobile Originated (Outgoing) Count
    long sms_mt_count;       // SMS Mobile Terminated (Incoming) Count
    long total_download;     // MB Downloaded
    long total_upload;       // MB Uploaded
} OperatorStats;

typedef struct OpNode
{
    char *operator_id;   // hash map (key) operator_id
    OperatorStats stats; // Hash map (Value)
    struct OpNode *next; // Chaining (linked list)
} OpNode;

/* ============================================================
   Function Declarations
   ============================================================ */

// Thread entry point
void* intopbillprocess(void *arg);

// Main processing function
void InteroperatorBillingProcess(const char *input_path, const char *output_path);

// Search and display functions
void search_operator(int client_fd, const char *filename, const char *operator_name);
void display_interoperator_billing_file(int client_fd, const char *filename);

// Hash map operations
unsigned long str_hash(const char *s);
OpNode* get_or_create_opnode(const char *operator_id, const char *operator_name);

// Utility functions
void chomp(char *s);
int split_pipe(char *line, char **tokens, int max_tokens);
long to_long_or_zero(const char *s);

// Line processing
void process_line(char *line);

#endif // INTOPBILLPROCESS_H
