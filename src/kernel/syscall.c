#include "syscall.h"
#include "log.h"
#include "thread.h"
#include "vfs.h" // For future use in file syscalls
#include "fb.h" // For framebuffer info
#include "event.h" // For event queue
#include <string.h> // For memcpy

// Syscall function pointer type
typedef void (*syscall_func_t)(struct registers* regs);

// Syscall table
static syscall_func_t syscall_table[256];

// --- Syscall Implementations ---

static void sys_exit(struct registers* regs) {
    klog(LOG_INFO, "Syscall: exit");
    thread_exit();
}

static void sys_write(struct registers* regs) {
    if (regs->rdi == 1) { // stdout
        char* msg = (char*)regs->rsi;
        klog(LOG_INFO, msg);
    }
}

// STUB for sys_open
static void sys_open(struct registers* regs) {
    klog(LOG_INFO, "Syscall: open (stub)");
    regs->rax = -1; // Return error for now
}

// STUB for sys_read
static void sys_read(struct registers* regs) {
    klog(LOG_INFO, "Syscall: read (stub)");
    regs->rax = -1;
}

// STUB for sys_stat
static void sys_stat(struct registers* regs) {
    klog(LOG_INFO, "Syscall: stat (stub)");
    regs->rax = -1;
}

static void sys_gfx_get_fb_info(struct registers* regs) {
    // Userspace provides a pointer to a fb_info_t struct
    fb_info_t* user_fb_info = (fb_info_t*)regs->rdi;
    const fb_info_t* kernel_fb_info = fb_get_info();
    
    // In a real scenario, we would need to verify the user pointer
    // and copy the data carefully.
    memcpy(user_fb_info, kernel_fb_info, sizeof(fb_info_t));
    
    regs->rax = 0; // Success
}

static void sys_input_poll_event(struct registers* regs) {
    // Userspace provides a pointer to an event_t struct
    event_t* user_event = (event_t*)regs->rdi;
    
    if (event_pop(user_event)) {
        regs->rax = 1; // Event was popped
    } else {
        regs->rax = 0; // No event was available
    }
}

void syscall_init() {
    // Initialize all syscalls to a default handler
    for (int i = 0; i < 256; i++) {
        // syscall_table[i] = some_default_handler;
    }

    // Register our syscalls
    syscall_table[SYS_EXIT] = sys_exit;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_READ] = sys_read;
    syscall_table[SYS_STAT] = sys_stat;
    syscall_table[SYS_GFX_GET_FB_INFO] = sys_gfx_get_fb_info;
    syscall_table[SYS_INPUT_POLL_EVENT] = sys_input_poll_event;
    
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
