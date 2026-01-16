#include "thread.h"
#include "heap.h"
#include "isr.h"
#include "log.h"
#include "scheduler.h"
#include "vfs.h"
#include "pmm.h" // For pmm_alloc_page
#include "vmm.h" // For vmm_map_page, PAGE_PRESENT, PAGE_WRITE, PAGE_USER
#include <stddef.h> // for NULL

static uint64_t next_thread_id = 0;
extern thread_t *current_thread;

// This function is called from assembly when a thread starts for the first time
void thread_entry(thread_func_t func, void *arg) {
  // Re-enable interrupts for the new thread
  enable_interrupts();

  // Call the actual thread function
  func(arg);

  // If the thread function returns, exit the thread
  thread_exit();
}

extern pml4_t* kernel_pml4;

void thread_init() {
  klog(LOG_INFO, "Thread: Initializing...");
  // Create a thread structure for the main kernel thread (the
  // kmain)
  current_thread = (thread_t *)kmalloc(sizeof(thread_t));
  if (!current_thread) {
    panic("Failed to allocate main kernel thread!", NULL);
  }
  current_thread->id = next_thread_id++;
  current_thread->state = THREAD_RUNNING;
  current_thread->stack = NULL; // Kernel main stack is managed separately
  current_thread->user_stack_base = NULL; // No userspace stack for kernel thread
  current_thread->pml4 = vmm_get_current_pml4(); // Kernel thread uses kernel pml4
  // Save the current RSP for the initial kernel thread
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(current_thread->rsp));
  current_thread->next = NULL;

  for (int i = 0; i < MAX_FILES; i++) {
    current_thread->fd_table[i].type = FD_TYPE_NONE;
    current_thread->fd_table[i].data.file.node = NULL;
    current_thread->fd_table[i].data.file.offset = 0;
    current_thread->fd_table[i].data.file.flags = 0;
    current_thread->fd_table[i].data.sock = NULL;
  }

  scheduler_init();
}

extern void thread_starter();

thread_t *thread_create(thread_func_t func, void *arg) {
  disable_interrupts();

  thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
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

  thread->user_stack_base = NULL; // No userspace stack for kernel thread
  thread->pml4 = vmm_get_current_pml4();   // Kernel thread uses kernel pml4

  thread->id = next_thread_id++;
  thread->state = THREAD_READY;

  for (int i = 0; i < MAX_FILES; i++) {
    thread->fd_table[i].type = FD_TYPE_NONE;
    thread->fd_table[i].data.file.node = NULL;
    thread->fd_table[i].data.file.offset = 0;
    thread->fd_table[i].data.file.flags = 0;
    thread->fd_table[i].data.sock = NULL;
  }

  // Set up the initial stack for the new thread
  uint64_t *stack_ptr =
      (uint64_t *)((uint64_t)thread->stack + KERNEL_STACK_SIZE);

  // Initial arguments for thread_entry, which are passed to thread_starter on the stack.
  *--stack_ptr = (uint64_t)arg;
  *--stack_ptr = (uint64_t)func;

  // The 'return address' for thread_switch is thread_starter.
  *--stack_ptr = (uint64_t)thread_starter;

  // Fake callee-saved registers for thread_switch to pop.
  // The order of these pushes must result in a stack layout that matches
  // the reverse of the 'pop' sequence in thread_switch.
  // thread_switch pops: r15, r14, r13, r12, rbx, rbp
  // So, we push fake values for rbp, rbx, r12, r13, r14, r15.
  // Since we decrement the stack pointer before writing, we push rbp first,
  // then rbx, and so on, so that r15 is at the lowest address (top of stack).
  *--stack_ptr = 0; // rbp
  *--stack_ptr = 0; // rbx
  *--stack_ptr = 0; // r12
  *--stack_ptr = 0; // r13
  *--stack_ptr = 0; // r14
  *--stack_ptr = 0; // r15

  thread->rsp = (uint64_t)stack_ptr;

  scheduler_add_thread(thread);

  enable_interrupts();
  return thread;
}

extern void userspace_trampoline();

// New function for userspace threads
thread_t* thread_create_userspace(uint64_t entry_point, pml4_t* pml4) {
    disable_interrupts();

    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
    if (!thread) {
        klog(LOG_ERROR, "Failed to allocate userspace thread structure.");
        enable_interrupts();
        return NULL;
    }
    thread->pml4 = pml4;

    // Allocate userspace stack
    void* user_stack_vaddr_start = (void*)(USER_STACK_TOP - USER_STACK_SIZE);
    klog(LOG_DEBUG, "Userspace stack: vaddr_start = %p, size = %u", user_stack_vaddr_start, USER_STACK_SIZE);

    for (uint64_t i = 0; i < USER_STACK_SIZE; i += PAGE_SIZE) {
        void* phys_page = pmm_alloc_page();
        if (!phys_page) {
            klog(LOG_ERROR, "Failed to allocate physical page for userspace stack.");
            vmm_destroy_address_space(thread->pml4);
            kfree(thread);
            enable_interrupts();
            return NULL;
        }
        vmm_map_page(thread->pml4, (void*)((uint64_t)user_stack_vaddr_start + i), phys_page, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    thread->user_stack_base = user_stack_vaddr_start;

    // Also allocate a kernel stack for syscalls/interrupts
    thread->stack = kmalloc(KERNEL_STACK_SIZE);
    if (!thread->stack) {
        klog(LOG_ERROR, "Failed to allocate kernel stack for userspace thread.");
        
        // The user stack is part of the address space, which we are about to destroy.
        // We just need to free the physical pages.
        for (uint64_t i = 0; i < USER_STACK_SIZE; i += PAGE_SIZE) {
            void* vaddr = (void*)((uint64_t)user_stack_vaddr_start + i);
            void* phys_addr = vmm_unmap_page(thread->pml4, vaddr);
            if (phys_addr) {
                pmm_free_page(phys_addr);
            }
        }
        vmm_destroy_address_space(thread->pml4);
        kfree(thread);
        enable_interrupts();
        return NULL;
    }

    thread->id = next_thread_id++;
    thread->state = THREAD_READY;

    for (int i = 0; i < MAX_FILES; i++) {
        thread->fd_table[i].type = FD_TYPE_NONE;
        thread->fd_table[i].data.file.node = NULL;
        thread->fd_table[i].data.file.offset = 0;
        thread->fd_table[i].data.file.flags = 0;
        thread->fd_table[i].data.sock = NULL;
    }

    // Set up the initial KERNEL stack for the new userspace thread.
    // This stack is what `thread_switch` will restore. It needs to be
    // crafted to eventually `iretq` to userspace.
    uint64_t *stack_ptr = (uint64_t *)((uint64_t)thread->stack + KERNEL_STACK_SIZE);

    klog(LOG_DEBUG, "Userspace IRETQ frame setup: entry_point = %p, USER_STACK_TOP = %p", (void*)entry_point, (void*)USER_STACK_TOP);
    // IRETQ frame
    *--stack_ptr = 0x23;                      // SS (User Data Segment)
    *--stack_ptr = USER_STACK_TOP;            // RSP (User Stack)
    *--stack_ptr = 0x202;                     // RFLAGS (Interrupts enabled)
    *--stack_ptr = 0x1B;                      // CS (User Code Segment)
    *--stack_ptr = entry_point;               // RIP

    // The address that `thread_switch` will `ret` to.
    // This trampoline just contains `iretq`.
    *--stack_ptr = (uint64_t)userspace_trampoline;

    // Fake callee-saved registers for thread_switch to pop
    *--stack_ptr = 0; // rbp
    *--stack_ptr = 0; // rbx
    *--stack_ptr = 0; // r12
    *--stack_ptr = 0; // r13
    *--stack_ptr = 0; // r14
    *--stack_ptr = 0; // r15

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
