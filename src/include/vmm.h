#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)

// Page Map Level 4
typedef struct {
    uint64_t entries[512];
} pml4_t;

// Page Directory Pointer Table
typedef struct {
    uint64_t entries[512];
} pdpt_t;

// Page Directory
typedef struct {
    uint64_t entries[512];
} pd_t;

// Page Table
typedef struct {
    uint64_t entries[512];
} pt_t;

void vmm_init();
void vmm_map_page(void* virt, void* phys, uint64_t flags);
void vmm_unmap_page(void* virt);

#endif // VMM_H
