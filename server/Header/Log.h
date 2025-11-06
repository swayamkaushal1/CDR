#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

// Log levels - ordered by priority
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_FATAL = 3
} LogLevel;

// Log configuration
typedef struct {
    FILE *log_file;
    LogLevel min_level;
    int console_output;
    pthread_mutex_t lock;
} LogConfig;

// Global log configuration
extern LogConfig g_log_config;

// Initialize logging system
// Parameters:
//   log_filename: Name of the log file (e.g., "server.log")
//   min_level: Minimum log level to record
//   enable_console: 1 to also print to console, 0 for file only
int log_init(const char *log_filename, LogLevel min_level, int enable_console);

// Close logging system
void log_cleanup(void);

// Core logging function
void log_message(LogLevel level, const char *file, int line, const char *func, const char *fmt, ...);

// Convenience macros for logging with automatic file, line, and function info
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

// High-level logging functions for common events
void log_connection_event(const char *ip_address, const char *action);
void log_auth_event(const char *email, const char *action, int success);
void log_menu_choice(const char *email, const char *menu_name, const char *choice);
void log_processing_event(const char *email, const char *operation, const char *status);
void log_search_event(const char *email, const char *search_type, const char *search_value, int found);
void log_file_operation(const char *email, const char *filename, const char *operation);

#endif // LOG_H
