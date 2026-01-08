#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "multiboot2.h"

#define PAGE_SIZE 4096

// Initialize the physical memory manager
void pmm_init(uint32_t multiboot_info_addr);

// Allocate a single physical page frame
void* pmm_alloc_page();

// Free a single physical page frame
void pmm_free_page(void* p);

#endif // PMM_H
