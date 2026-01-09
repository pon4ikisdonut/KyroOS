#ifndef LOG_H
#define LOG_H

#include "isr.h" // For struct registers

// Log levels
typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_PANIC,
} log_level_t;

void log_init();
void klog(log_level_t level, const char* message);
void panic(const char* message, struct registers* regs);
void klog_print_hex(uint64_t val); // Add this declaration as it's used by other files now
void klog_putchar(char c);
void klog_print_str(const char* str);

#endif // LOG_H