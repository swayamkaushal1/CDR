// CustBillProcess.c - Customer billing CDR processing
#include "../Header/CustBillProcess.h"

/* ============================================================
   Static Variables
   ============================================================ */

static Customer *hashTable[HASH_SIZE];
static int totalRecords = 0;

/* ============================================================
   Hash Function
   ============================================================ */

unsigned int hashFunction(long key)
{
    return (unsigned int)(key % HASH_SIZE);
}

/* ============================================================
   Customer Management Functions
   ============================================================ */

Customer* createCustomer(long msisdn, const char *operatorName, int operatorCode)
{
    Customer *cust = (Customer *)malloc(sizeof(Customer));
    if (!cust) return NULL;
    
    // Initialize customer data
    cust->msisdn = msisdn;
    strncpy(cust->operatorName, operatorName, sizeof(cust->operatorName) - 1);
    cust->operatorName[sizeof(cust->operatorName) - 1] = '\0';
    cust->operatorCode = operatorCode;
    
    // Initialize all counters to zero
    cust->inVoiceWithin = cust->outVoiceWithin = 0;
    cust->inVoiceOutside = cust->outVoiceOutside = 0;
    cust->smsInWithin = cust->smsOutWithin = 0;
    cust->smsInOutside = cust->smsOutOutside = 0;
    cust->mbDownload = cust->mbUpload = 0;
    cust->next = NULL;
    
    return cust;
}

Customer* getCustomer(long msisdn, const char *operatorName, int operatorCode)
{
    unsigned int index = hashFunction(msisdn);
    Customer *curr = hashTable[index];
    
    // Search for existing customer in chain
    while (curr) {
        if (curr->msisdn == msisdn)
            return curr;
        curr = curr->next;
    }
    
    // Customer not found - create new one and add to hash table
    Customer *newCust = createCustomer(msisdn, operatorName, operatorCode);
    if (newCust) {
        newCust->next = hashTable[index];
        hashTable[index] = newCust;
    }
    
    return newCust;
}

/* ============================================================
   Helper Functions (Internal)
   ============================================================ */

static void updateCustomerStats(Customer *cust, const char *callType, 
                                int sameOperator, float duration, 
                                float download, float upload)
{
    if (strcmp(callType, "MOC") == 0) {
        sameOperator ? (cust->outVoiceWithin += duration) 
                    : (cust->outVoiceOutside += duration);
    }
    else if (strcmp(callType, "MTC") == 0) {
        sameOperator ? (cust->inVoiceWithin += duration) 
                    : (cust->inVoiceOutside += duration);
    }
    else if (strcmp(callType, "SMS-MO") == 0) {
        sameOperator ? cust->smsOutWithin++ : cust->smsOutOutside++;
    }
    else if (strcmp(callType, "SMS-MT") == 0) {
        sameOperator ? cust->smsInWithin++ : cust->smsInOutside++;
    }
    else if (strcmp(callType, "GPRS") == 0) {
        cust->mbDownload += download;
        cust->mbUpload += upload;
    }
}

/* ============================================================
   CDR File Processing
   ============================================================ */

void processCDRFile(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error opening CDR file '%s': %s\n", filename, strerror(errno));
        return;
    }
    
    char line[512];
    totalRecords = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline characters
        line[strcspn(line, "\r\n")] = 0;
        
        // Initialize CDR fields
        long msisdn = 0, thirdPartyMsisdn = 0;
        char opName[64] = {0}, callType[16] = {0};
        int opCode = 0, thirdPartyOpCode = 0;
        float duration = 0, download = 0, upload = 0;
        
        // Parse CDR line (9 fields expected)
        int matched = sscanf(line, "%ld|%63[^|]|%d|%15[^|]|%f|%f|%f|%ld|%d",
                            &msisdn, opName, &opCode, callType,
                            &duration, &download, &upload,
                            &thirdPartyMsisdn, &thirdPartyOpCode);
        
        // Handle edge case: GPRS records with empty third party MSISDN (||)
        if (matched < 9) {
            matched = sscanf(line, "%ld|%63[^|]|%d|%15[^|]|%f|%f|%f||%d",
                            &msisdn, opName, &opCode, callType,
                            &duration, &download, &upload, &thirdPartyOpCode);
            if (matched < 8) continue; // Skip invalid lines
        }
        
        // Get or create customer record
        Customer *cust = getCustomer(msisdn, opName, opCode);
        if (!cust) continue;
        
        // Determine if call is within same operator
        int sameOperator = (opCode == thirdPartyOpCode);
        
        // Update customer statistics
        updateCustomerStats(cust, callType, sameOperator, duration, download, upload);
        
        totalRecords++;
    }
    
    fclose(fp);
}

static void writeCustomerRecord(FILE *fp, Customer *cust)
{
    fprintf(fp, "\nCustomer ID: %ld (%s)\n", cust->msisdn, cust->operatorName);
    fprintf(fp, "* Services within the mobile operator *\n");
    fprintf(fp, "Incoming voice call durations: %.2f\n", cust->inVoiceWithin);
    fprintf(fp, "Outgoing voice call durations: %.2f\n", cust->outVoiceWithin);
    fprintf(fp, "Incoming SMS messages: %d\n", cust->smsInWithin);
    fprintf(fp, "Outgoing SMS messages: %d\n", cust->smsOutWithin);
    fprintf(fp, "* Services outside the mobile operator *\n");
    fprintf(fp, "Incoming voice call durations: %.2f\n", cust->inVoiceOutside);
    fprintf(fp, "Outgoing voice call durations: %.2f\n", cust->outVoiceOutside);
    fprintf(fp, "Incoming SMS messages: %d\n", cust->smsInOutside);
    fprintf(fp, "Outgoing SMS messages: %d\n", cust->smsOutOutside);
    fprintf(fp, "* Internet use *\n");
    fprintf(fp, "MB downloaded: %.2f | MB uploaded: %.2f\n",
            cust->mbDownload, cust->mbUpload);
    fprintf(fp, "----------------------------------------\n");
}

/* ============================================================
   Output Generation
   ============================================================ */

void writeCBFile(const char *outputFile)
{
    FILE *fp = fopen(outputFile, "w");
    if (!fp) {
        fprintf(stderr, "Error creating output file '%s': %s\n", outputFile, strerror(errno));
        return;
    }
    
    fprintf(fp, "#Customers Data Base:\n");
    
    // Iterate through hash table and write all customer records
    int customerCount = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        Customer *cust = hashTable[i];
        while (cust) {
            writeCustomerRecord(fp, cust);
            customerCount++;
            cust = cust->next;
        }
    }
    
    fclose(fp);
}

/* ============================================================
   Memory Management
   ============================================================ */

void cleanupHashTable(void)
{
    for (int i = 0; i < HASH_SIZE; i++) {
        Customer *cust = hashTable[i];
        while (cust) {
            Customer *temp = cust;
            cust = cust->next;
            free(temp);
        }
        hashTable[i] = NULL;
    }
}

/* ============================================================
   Thread Entry Point
   ============================================================ */

void* custbillprocess(void *arg)
{
    ProcessThreadArg *threadArg = (ProcessThreadArg *)arg;
    
    // Build file paths
    const char *inputPath = "data/data.cdr";
    char outputPath[300];
    snprintf(outputPath, sizeof(outputPath), "%s/CB.txt", 
             threadArg ? threadArg->output_dir : "Output");
    
    // Initialize hash table to NULL
    for (int i = 0; i < HASH_SIZE; i++)
        hashTable[i] = NULL;
    
    // Process CDR file and aggregate customer data
    processCDRFile(inputPath);
    
    // Write customer billing report
    writeCBFile(outputPath);
    
    // Free allocated memory
    cleanupHashTable();
    
    return NULL;
}
