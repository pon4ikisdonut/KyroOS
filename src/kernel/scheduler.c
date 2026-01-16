#include "scheduler.h"
#include "heap.h"
#include "isr.h"
#include "log.h"
#include "thread.h"
#include "pmm.h" // For pmm_free_page
#include "vmm.h" // For vmm_unmap_page, vmm_destroy_address_space, PAGE_SIZE
#include <stddef.h> // for NULL

// Simple round-robin scheduler

static thread_t *ready_queue = NULL; // Head of the ready queue
thread_t *current_thread = NULL;

void scheduler_init() {
  klog(LOG_INFO, "Scheduler: Initializing...");
  // The main kernel thread is already running, no need to add it to the ready
  // queue. `current_thread` is initialized in thread_init()
  ready_queue = NULL;
  klog(LOG_INFO, "Scheduler initialized.");
}

void scheduler_add_thread(thread_t *thread) {
  if (!thread)
    return;

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

thread_t *get_current_thread() { return current_thread; }

// The core scheduler function
void schedule() {
  disable_interrupts();

  if (!current_thread) {
    enable_interrupts();
    return; // Nothing to schedule
  }

  thread_t *old_thread = current_thread;
  thread_t *next_thread = NULL;

  if (ready_queue) {
    next_thread = ready_queue->next;

    // Find a ready thread
    while (next_thread->state != THREAD_READY &&
           next_thread != ready_queue->next) {
      if (next_thread->state == THREAD_DEAD) {
        // Clean up the dead thread
        thread_t* dead_thread = next_thread;
        next_thread = next_thread->next;
        
        // Unlink from ready queue
        // Find the thread before the dead one
        thread_t* prev = ready_queue;
        while(prev->next != dead_thread) {
            prev = prev->next;
        }
        prev->next = next_thread;
        if (ready_queue == dead_thread) {
            ready_queue = prev;
        }
        
        // Free user stack
        if (dead_thread->user_stack_base) {
            for (uint64_t i = 0; i < USER_STACK_SIZE; i += PAGE_SIZE) {
                void* vaddr = (void*)((uint64_t)dead_thread->user_stack_base + i);
                void* phys_addr = vmm_unmap_page(dead_thread->pml4, vaddr);
                if (phys_addr) {
                    pmm_free_page(phys_addr);
                }
            }
        }
        
        // Free address space
        if (dead_thread->pml4) {
            // Don't destroy kernel address space
            if (dead_thread->id != 0) { // Assuming kernel thread has id 0
                 vmm_destroy_address_space(dead_thread->pml4);
            }
        }

        // Free kernel stack
        if (dead_thread->stack) {
            kfree(dead_thread->stack);
        }

        // Free thread struct
        kfree(dead_thread);

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

  ready_queue =
      next_thread; // The new head of the ready queue is the next thread to run.

  if (old_thread->state == THREAD_RUNNING) {
    old_thread->state = THREAD_READY;
  }

  next_thread->state = THREAD_RUNNING;
  current_thread = next_thread;

  if (old_thread != next_thread) {
    klog(LOG_DEBUG, "Scheduler: Switching from %p (ID: %d) to %p (ID: %d)", old_thread, old_thread->id, next_thread, next_thread->id);
    thread_switch(old_thread, next_thread);
  }

  enable_interrupts();
}
