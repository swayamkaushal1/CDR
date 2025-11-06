#ifndef CUSTBILLPROCESS_H
#define CUSTBILLPROCESS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================
   Constants
   ============================================================ */
#define HASH_SIZE 1000

/* ============================================================
   Data Structures
   ============================================================ */

// Customer structure for billing
typedef struct Customer {
    long msisdn;
    char operatorName[64];
    int operatorCode;
    
    // Voice call durations (within and outside operator)
    float inVoiceWithin;
    float outVoiceWithin;
    float inVoiceOutside;
    float outVoiceOutside;
    
    // SMS counts
    int smsInWithin;
    int smsOutWithin;
    int smsInOutside;
    int smsOutOutside;
    
    // Data usage
    float mbDownload;
    float mbUpload;
    
    struct Customer *next; // for hash collision chaining
} Customer;

// Thread argument structure for passing output directory
typedef struct {
    char output_dir[256];
} ProcessThreadArg;

/* ============================================================
   Function Declarations
   ============================================================ */

// Thread entry point
void* custbillprocess(void *arg);

// Search and display functions
void search_msisdn(int client_fd, const char *filename, long msisdn);
void display_customer_billing_file(int client_fd, const char *filename);

// Customer processing functions
Customer* createCustomer(long msisdn, const char *operatorName, int operatorCode);
Customer* getCustomer(long msisdn, const char *operatorName, int operatorCode);

// CDR processing functions
void processCDRFile(const char *filename);
void writeCBFile(const char *outputFile);
void cleanupHashTable(void);

// Hash function
unsigned int hashFunction(long key);

#endif // CUSTBILLPROCESS_H
