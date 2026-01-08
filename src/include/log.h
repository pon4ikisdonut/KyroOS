#ifndef LOG_H
#define LOG_H

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

#endif // LOG_H
