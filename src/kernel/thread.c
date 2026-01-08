#include "thread.h"
#include "scheduler.h"
#include "heap.h"
#include "log.h"
#include "isr.h"
#include <stddef.h> // for NULL

static uint64_t next_thread_id = 0;
extern thread_t* current_thread;

// This function is called from assembly when a thread starts for the first time
void thread_entry(thread_func_t func, void* arg) {
    // Re-enable interrupts for the new thread
    enable_interrupts();
    
    // Call the actual thread function
    func(arg);
    
    // If the thread function returns, exit the thread
    thread_exit();
}

void thread_init() {
    // Create a thread structure for the main kernel thread (the one running kmain)
    current_thread = (thread_t*)kmalloc(sizeof(thread_t));
    if (!current_thread) {
        panic("Failed to allocate main kernel thread!", NULL);
    }
    current_thread->id = next_thread_id++;
    current_thread->state = THREAD_RUNNING;
    current_thread->stack = NULL; // Kernel main stack is managed separately
    current_thread->next = NULL;
    
    scheduler_init();
}

thread_t* thread_create(thread_func_t func, void* arg) {
    disable_interrupts();

    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t));
    if (!thread) {
        klog(LOG_ERROR, "Failed to allocate thread structure.");
        enable_interrupts();
        return NULL;
    }

    thread->stack = kmalloc(KERNEL_STACK_SIZE);
    if (!thread->stack) {
        klog(LOG_ERROR, "Failed to allocate thread stack.");
        kfree(thread);
        enable_interrupts();
        return NULL;
    }

    thread->id = next_thread_id++;
    thread->state = THREAD_READY;

    // Set up the initial stack for the new thread
    uint64_t* stack_ptr = (uint64_t*)((uint64_t)thread->stack + KERNEL_STACK_SIZE);

    // Fake IRETQ frame
    *--stack_ptr = 0x10; // SS (Kernel Data Segment)
    *--stack_ptr = (uint64_t)thread->stack + KERNEL_STACK_SIZE; // RSP
    *--stack_ptr = 0x202; // RFLAGS (interrupts enabled)
    *--stack_ptr = 0x08; // CS (Kernel Code Segment)
    *--stack_ptr = (uint64_t)thread_entry; // RIP

    // Fake PUSH_ALL frame (from isr_stubs.asm)
    *--stack_ptr = (uint64_t)arg;  // RDI (argument for thread_entry)
    *--stack_ptr = (uint64_t)func; // RSI (argument for thread_entry)
    for (int i = 0; i < 14; i++) { // R15, R14... RAX (except RDI, RSI)
        *--stack_ptr = 0;
    }
    
    thread->rsp = (uint64_t)stack_ptr;
    
    scheduler_add_thread(thread);

    enable_interrupts();
    return thread;
}

void thread_exit() {
    disable_interrupts();
    get_current_thread()->state = THREAD_DEAD;
    klog(LOG_INFO, "Thread exited.");
    // The scheduler will handle cleaning up the dead thread
    schedule();
    // We should never get here
    panic("Returned to a dead thread!", NULL);
}
