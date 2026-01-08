#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include "isr.h" // For struct registers

#define KERNEL_STACK_SIZE 8192 // 8KB stack for kernel threads

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

typedef struct thread {
    uint64_t rsp; // Stack pointer
    void* stack; // Pointer to the bottom of the stack
    thread_state_t state;
    uint64_t id;
    struct thread* next;
} thread_t;

// Function pointer for thread entry point
typedef void (*thread_func_t)(void*);

void thread_init();
thread_t* thread_create(thread_func_t func, void* arg);
void thread_exit();

// Assembly function for context switching
void thread_switch(thread_t* old_thread, thread_t* new_thread);

#endif // THREAD_H
