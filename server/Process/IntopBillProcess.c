// IntopBillProcess.c - Interoperator billing CDR processing
#include "../Header/IntopBillProcess.h"
#include "../Header/CustBillProcess.h" // for ProcessThreadArg

/* ============================================================
   Static Variables
   ============================================================ */

static OpNode *buckets[NUM_BUCKETS] = {NULL};

/* ============================================================
   Hash Map Implementation
   ============================================================ */

unsigned long str_hash(const char *s)
{
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*s++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

OpNode *get_or_create_opnode(const char *operator_id, const char *operator_name)
{
    unsigned long h = str_hash(operator_id);
    unsigned idx = (unsigned)(h % NUM_BUCKETS);
    OpNode *node = buckets[idx];

    while (node)
    {
        if (strcmp(node->operator_id, operator_id) == 0)
            return node;
        node = node->next;
    }

    // Create a new node
    OpNode *newnode = (OpNode *)calloc(1, sizeof(OpNode));
    newnode->operator_id = strdup(operator_id);
    newnode->stats.operator_name = operator_name ? strdup(operator_name) : strdup("UNKNOWN");
    newnode->next = buckets[idx];
    buckets[idx] = newnode;
    return newnode;
}

/* ============================================================
   Utility Functions
   ============================================================ */

void chomp(char *s)
{
    if (!s) return;
    
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

int split_pipe(char *line, char **tokens, int max_tokens)
{
    int idx = 0;
    char *start = line;
    for (char *p = line; *p && idx < max_tokens - 1; p++)
    {
        if (*p == '|')
        {
            *p = '\0';
            tokens[idx++] = start;
            start = p + 1;
        }
    }
    tokens[idx++] = start;
    return idx;
}

long to_long_or_zero(const char *s)
{
    if (!s || *s == '\0') return 0;
    
    char *endptr;
    long v = strtol(s, &endptr, 10);
    
    return (endptr == s) ? 0 : v;
}

/* ============================================================
   CDR Line Processor
   ============================================================ */

void process_line(char *line)
{
    chomp(line);
    if (line[0] == '\0') return;

    // Parse CDR line into tokens
    char *tokens[12];
    int n = split_pipe(line, tokens, 9);
    
    // Fill missing tokens with empty strings
    for (int i = n; i < 9; ++i)
        tokens[i] = "";

    // Extract fields
    const char *operator_name = tokens[1];
    const char *operator_id = tokens[2];
    const char *call_type = tokens[3];
    const char *duration_s = tokens[4];
    const char *download_s = tokens[5];
    const char *upload_s = tokens[6];

    // Validate operator_id
    if (!operator_id || operator_id[0] == '\0') return;
    if (!call_type) return;

    // Get or create operator node
    OpNode *node = get_or_create_opnode(operator_id, operator_name);
    OperatorStats *stats = &node->stats;

    // Normalize call type to uppercase
    char call_type_upper[32];
    snprintf(call_type_upper, sizeof(call_type_upper), "%s", call_type);
    for (char *p = call_type_upper; *p; ++p)
        *p = toupper((unsigned char)*p);

    // Update statistics based on call type
    if (strcmp(call_type_upper, "MOC") == 0)
        stats->total_moc_duration += to_long_or_zero(duration_s);
    else if (strcmp(call_type_upper, "MTC") == 0)
        stats->total_mtc_duration += to_long_or_zero(duration_s);
    else if (strcmp(call_type_upper, "SMS-MO") == 0)
        stats->sms_mo_count++;
    else if (strcmp(call_type_upper, "SMS-MT") == 0)
        stats->sms_mt_count++;
    else if (strcmp(call_type_upper, "GPRS") == 0) {
        stats->total_download += to_long_or_zero(download_s);
        stats->total_upload += to_long_or_zero(upload_s);
    }
}

/* ============================================================
   Helper Functions for Main Processing
   ============================================================ */

static void write_billing_output(FILE *fout)
{
    for (unsigned i = 0; i < NUM_BUCKETS; ++i) {
        OpNode *node = buckets[i];
        while (node) {
            OperatorStats *stats = &node->stats;
            fprintf(fout, "Operator Brand: %s (%s)\n", stats->operator_name, node->operator_id);
            fprintf(fout, "\tIncoming voice call durations: %ld\n", stats->total_mtc_duration);
            fprintf(fout, "\tOutgoing voice call durations: %ld\n", stats->total_moc_duration);
            fprintf(fout, "\tIncoming SMS messages: %ld\n", stats->sms_mt_count);
            fprintf(fout, "\tOutgoing SMS messages: %ld\n", stats->sms_mo_count);
            fprintf(fout, "\tMB Download: %ld | MB Uploaded: %ld\n", 
                    stats->total_download, stats->total_upload);
            fprintf(fout, "----------------------------------------\n");
            node = node->next;
        }
    }
}

static void cleanup_hash_table(void)
{
    for (unsigned i = 0; i < NUM_BUCKETS; ++i) {
        OpNode *node = buckets[i];
        while (node) {
            OpNode *tmp = node->next;
            free(node->operator_id);
            free(node->stats.operator_name);
            free(node);
            node = tmp;
        }
        buckets[i] = NULL;
    }
}

/* ============================================================
   Main Processing Function
   ============================================================ */

void InteroperatorBillingProcess(const char *input_path, const char *output_path)
{
    // Open input file
    FILE *fin = fopen(input_path, "r");
    if (!fin) {
        fprintf(stderr, "Error opening input file '%s': %s\n", input_path, strerror(errno));
        return;
    }

    // Open output file
    FILE *fout = fopen(output_path, "w");
    if (!fout) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_path, strerror(errno));
        fclose(fin);
        return;
    }

    // Process CDR file line by line
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fin)) != -1) {
        process_line(line);
    }

    free(line);
    fclose(fin);

    // Write aggregated results to output file
    write_billing_output(fout);
    fclose(fout);

    // Cleanup allocated memory
    cleanup_hash_table();
}

/* ============================================================
   Thread Entry Point
   ============================================================ */

void* intopbillprocess(void *arg)
{
    ProcessThreadArg *threadArg = (ProcessThreadArg *)arg;
    
    // Build file paths
    const char *input_file = "data/data.cdr";
    char output_file[512];
    snprintf(output_file, sizeof(output_file), "%s/IOSB.txt",
             threadArg ? threadArg->output_dir : "Output");
    
    // Process CDR and generate interoperator billing
    InteroperatorBillingProcess(input_file, output_file);
    
    return NULL;
}
