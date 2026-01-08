#include "tss.h"
#include "gdt.h"
#include "heap.h" // For kmalloc
#include "log.h"
#include <string.h> // For memset

#define KERNEL_TSS_STACK_SIZE 8192

static struct tss_entry_struct tss_entry;
static uint8_t* tss_kernel_stack;

// External assembly function to load the TSS register
extern void tss_flush();

void tss_init() {
    // Allocate a stack for the TSS to use when an interrupt comes from user mode
    tss_kernel_stack = (uint8_t*)kmalloc(KERNEL_TSS_STACK_SIZE);
    if (!tss_kernel_stack) {
        panic("TSS: Failed to allocate kernel stack.", NULL);
    }
    
    memset(&tss_entry, 0, sizeof(struct tss_entry_struct));

    // Set the kernel stack pointer for Ring 0
    tss_entry.rsp0 = (uint64_t)tss_kernel_stack + KERNEL_TSS_STACK_SIZE;
    
    // Set up the TSS descriptor in the GDT
    gdt_set_tss(&tss_entry);
    
    // Load the TSS register (ltr)
    // The selector for TSS is the 5th entry in our GDT
    tss_flush();
    
    klog(LOG_INFO, "TSS Initialized.");
}

void tss_set_stack(uint64_t stack) {
    tss_entry.rsp0 = stack;
}
