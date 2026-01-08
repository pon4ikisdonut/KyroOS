#include "scheduler.h"
#include "thread.h"
#include "log.h"
#include "isr.h"
#include "heap.h"
#include <stddef.h> // for NULL

// Simple round-robin scheduler

static thread_t* ready_queue = NULL; // Head of the ready queue
thread_t* current_thread = NULL;

void scheduler_init() {
    // The main kernel thread is already running, no need to add it to the ready queue.
    // `current_thread` is initialized in thread_init()
    ready_queue = NULL;
    klog(LOG_INFO, "Scheduler initialized.");
}

void scheduler_add_thread(thread_t* thread) {
    if (!thread) return;

    disable_interrupts();
    if (ready_queue == NULL) {
        ready_queue = thread;
        thread->next = thread; // Circular list
    } else {
        thread->next = ready_queue->next;
        ready_queue->next = thread;
        ready_queue = thread; // New thread becomes the tail
    }
    enable_interrupts();
}

thread_t* get_current_thread() {
    return current_thread;
}

// The core scheduler function
void schedule() {
    disable_interrupts();

    if (!current_thread) {
        enable_interrupts();
        return; // Nothing to schedule
    }

    thread_t* old_thread = current_thread;
    thread_t* next_thread = NULL;

    if (ready_queue) {
        next_thread = ready_queue->next;
        
        // Find a ready thread
        while(next_thread->state != THREAD_READY && next_thread != ready_queue->next) {
            if (next_thread->state == THREAD_DEAD) {
                // TODO: Clean up dead threads (free stack and thread struct)
                // For now, we just skip them. This is a memory leak.
                next_thread = next_thread->next;
            } else {
                next_thread = next_thread->next;
            }
        }

        if (next_thread->state != THREAD_READY) {
            // No ready threads found
            next_thread = NULL;
        }
    }
    
    if (!next_thread) {
        // No other ready threads, continue with the current one if it's running
        if (old_thread->state == THREAD_RUNNING) {
            enable_interrupts();
            return;
        } else {
            // Current thread is blocked/dead, but there's nothing else to run
            panic("Scheduler: No runnable threads!", NULL);
        }
    }

    ready_queue = next_thread; // The new head of the ready queue is the next thread to run.

    if (old_thread->state == THREAD_RUNNING) {
        old_thread->state = THREAD_READY;
    }
    
    next_thread->state = THREAD_RUNNING;
    current_thread = next_thread;

    if (old_thread != next_thread) {
        thread_switch(old_thread, next_thread);
    }
    
    enable_interrupts();
}
