#include "syscall.h"
#include "log.h"
#include "thread.h"

// Syscall function pointer type
typedef void (*syscall_func_t)(struct registers* regs);

// Syscall table
static syscall_func_t syscall_table[256];

// Syscall implementations
static void sys_exit(struct registers* regs) {
    klog(LOG_INFO, "Syscall: exit");
    thread_exit();
}

static void sys_write(struct registers* regs) {
    // For now, just print to the kernel log
    // regs->rdi would be the file descriptor
    // regs->rsi would be the buffer
    // regs->rdx would be the count
    char* msg = (char*)regs->rsi;
    klog(LOG_INFO, "Syscall: write");
    klog(LOG_INFO, msg);
}

void syscall_init() {
    // Initialize all syscalls to a default handler
    for (int i = 0; i < 256; i++) {
        // syscall_table[i] = some_default_handler;
    }

    // Register our syscalls
    syscall_table[SYS_EXIT] = sys_exit;
    syscall_table[SYS_WRITE] = sys_write;
    
    klog(LOG_INFO, "Syscall handler initialized.");
}

void syscall_handler(struct registers* regs) {
    uint64_t syscall_num = regs->rax;

    if (syscall_num < 256 && syscall_table[syscall_num] != 0) {
        syscall_func_t func = syscall_table[syscall_num];
        func(regs);
    } else {
        klog(LOG_WARN, "Unknown syscall number.");
    }
}
