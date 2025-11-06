#include "../Header/Log.h"
#include <sys/stat.h>
#include <libgen.h>

// Global log configuration
LogConfig g_log_config = {
    .log_file = NULL,
    .min_level = LOG_INFO,
    .console_output = 1,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

// Log level names
static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "FATAL"
};

// Log level colors for console output (ANSI codes)
static const char *level_colors[] = {
    "\033[36m",  // Cyan for DEBUG
    "\033[32m",  // Green for INFO
    "\033[33m",  // Yellow for WARN
    "\033[35m"   // Magenta for FATAL
};
static const char *color_reset = "\033[0m";

// Initialize logging system
int log_init(const char *log_filename, LogLevel min_level, int enable_console) {
    pthread_mutex_lock(&g_log_config.lock);
    
    // Close existing log file if open
    if (g_log_config.log_file != NULL && g_log_config.log_file != stdout) {
        fclose(g_log_config.log_file);
        g_log_config.log_file = NULL;
    }
    
    // Extract directory path and create it if it doesn't exist
    char *log_path_copy = strdup(log_filename);
    if (log_path_copy != NULL) {
        char *dir_path = dirname(log_path_copy);
        // Create directory (mkdir returns 0 on success, -1 if exists or error)
        // 0755 = rwxr-xr-x permissions
        mkdir(dir_path, 0755);
        free(log_path_copy);
    }
    
    // Open new log file
    g_log_config.log_file = fopen(log_filename, "a");
    if (g_log_config.log_file == NULL) {
        pthread_mutex_unlock(&g_log_config.lock);
        fprintf(stderr, "ERROR: Failed to open log file: %s\n", log_filename);
        return -1;
    }
    
    g_log_config.min_level = min_level;
    g_log_config.console_output = enable_console;
    
    pthread_mutex_unlock(&g_log_config.lock);
    
    // Log system initialization
    LOG_INFO("Logging system initialized - Log file: %s", log_filename);
    
    return 0;
}

// Close logging system
void log_cleanup(void) {
    pthread_mutex_lock(&g_log_config.lock);
    
    if (g_log_config.log_file != NULL && g_log_config.log_file != stdout) {
        LOG_INFO("Logging system shutting down");
        fclose(g_log_config.log_file);
        g_log_config.log_file = NULL;
    }
    
    pthread_mutex_unlock(&g_log_config.lock);
}

// Get current timestamp string
static void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Core logging function
void log_message(LogLevel level, const char *file, int line, const char *func, const char *fmt, ...) {
    // Check if this log level should be recorded
    if (level < g_log_config.min_level) {
        return;
    }
    
    pthread_mutex_lock(&g_log_config.lock);
    
    // Get timestamp
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Extract just the filename from full path
    const char *filename = strrchr(file, '/');
    if (filename == NULL) {
        filename = strrchr(file, '\\');
    }
    filename = (filename != NULL) ? filename + 1 : file;
    
    // Format the user message
    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    // Write to log file
    if (g_log_config.log_file != NULL) {
        fprintf(g_log_config.log_file, "[%s] [%-5s] [%s:%d:%s] %s\n",
                timestamp, level_names[level], filename, line, func, message);
        fflush(g_log_config.log_file);
    }
    
    // Write to console if enabled
    if (g_log_config.console_output) {
        printf("%s[%s] [%-5s]%s [%s:%d:%s] %s\n",
               level_colors[level], timestamp, level_names[level], color_reset,
               filename, line, func, message);
    }
    
    pthread_mutex_unlock(&g_log_config.lock);
}

// High-level logging function: Connection events
void log_connection_event(const char *ip_address, const char *action) {
    LOG_INFO("CONNECTION | IP: %s | Action: %s", ip_address, action);
}

// High-level logging function: Authentication events
void log_auth_event(const char *email, const char *action, int success) {
    if (success) {
        LOG_INFO("AUTH | User: %s | Action: %s | Status: SUCCESS", email, action);
    } else {
        LOG_WARN("AUTH | User: %s | Action: %s | Status: FAILED", email, action);
    }
}

// High-level logging function: Menu choices
void log_menu_choice(const char *email, const char *menu_name, const char *choice) {
    LOG_DEBUG("MENU | User: %s | Menu: %s | Choice: %s", email, menu_name, choice);
}

// High-level logging function: Processing events
void log_processing_event(const char *email, const char *operation, const char *status) {
    LOG_INFO("PROCESS | User: %s | Operation: %s | Status: %s", email, operation, status);
}

// High-level logging function: Search events
void log_search_event(const char *email, const char *search_type, const char *search_value, int found) {
    if (found) {
        LOG_INFO("SEARCH | User: %s | Type: %s | Value: %s | Result: FOUND", 
                 email, search_type, search_value);
    } else {
        LOG_WARN("SEARCH | User: %s | Type: %s | Value: %s | Result: NOT FOUND", 
                 email, search_type, search_value);
    }
}

// High-level logging function: File operations
void log_file_operation(const char *email, const char *filename, const char *operation) {
    LOG_INFO("FILE | User: %s | File: %s | Operation: %s", email, filename, operation);
}
