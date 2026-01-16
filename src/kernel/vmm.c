#include "vmm.h"
#include "pmm.h"
#include "log.h"
#include "kstring.h"
#include <stddef.h> // for NULL

// This is the global offset for the higher-half direct map, defined in kernel.c
// extern uint64_t hhdm_offset; // Moved to vmm.h

// #define P_TO_V(p) ((void*)((uint64_t)(p) + hhdm_offset)) // Moved to vmm.h
// #define V_TO_P(v) ((void*)((uint64_t)(v) - hhdm_offset)) // Moved to vmm.h

// Kernel's top-level page map (PML4)
// This is created in boot.asm and we get a pointer to it.
// extern pml4_t pml4[]; // No longer directly using the uninitialized pml4 from boot.asm
pml4_t* kernel_pml4;

void* vmm_phys_to_virt(void* phys_addr) {
    return (void*)((uint64_t)phys_addr + hhdm_offset);
}

void* vmm_virt_to_phys(void* virt_addr) {
    return (void*)((uint64_t)virt_addr - hhdm_offset);
}

void vmm_init() {
    uint64_t current_cr3_phys;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(current_cr3_phys));
    kernel_pml4 = (pml4_t*)vmm_phys_to_virt((void*)current_cr3_phys);
    klog(LOG_INFO, "VMM initialized. Kernel PML4 at %p.", kernel_pml4);
}

pml4_t* vmm_get_current_pml4() {
    uint64_t pml4_phys;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(pml4_phys));
    return (pml4_t*)vmm_phys_to_virt((void*)pml4_phys);
}

void vmm_switch_address_space(pml4_t* pml4) {
    uint64_t pml4_phys = (uint64_t)vmm_virt_to_phys(pml4);
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}

pml4_t* vmm_create_address_space() {
    pml4_t* new_pml4_virt = (pml4_t*)vmm_phys_to_virt(pmm_alloc_page());
    if (!new_pml4_virt) {
        klog(LOG_ERROR, "VMM: Failed to allocate page for new PML4.");
        return NULL;
    }
    klog(LOG_DEBUG, "VMM: new_pml4_virt allocated at %p", new_pml4_virt);

    // Clear the lower (user) half of the new page map
    klog(LOG_DEBUG, "VMM: Clearing lower half of new PML4...");
    memset(new_pml4_virt, 0, 256 * sizeof(uint64_t));
    klog(LOG_DEBUG, "VMM: Lower half cleared.");

    // Copy the upper (kernel) half of the page map
    klog(LOG_DEBUG, "VMM: Copying upper half of kernel PML4 from %p to %p...", &kernel_pml4->entries[256], &new_pml4_virt->entries[256]);
    memcpy(&new_pml4_virt->entries[256], &kernel_pml4->entries[256], 256 * sizeof(uint64_t));
    klog(LOG_DEBUG, "VMM: Upper half copied.");

    return new_pml4_virt;
}

void vmm_map_page(pml4_t* pml4_virt, void* virt, void* phys, uint64_t flags) {
    uint64_t virt_addr = (uint64_t)virt;
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;
    
    pdpt_t* pdpt_virt;
    if (!(pml4_virt->entries[pml4_index] & PAGE_PRESENT)) {
        void* new_table_phys = pmm_alloc_page();
        if (!new_table_phys) panic("VMM: Out of memory for PDPT!", NULL);
        pdpt_virt = (pdpt_t*)vmm_phys_to_virt((void*)new_table_phys);
        memset(pdpt_virt, 0, PAGE_SIZE);
        pml4_virt->entries[pml4_index] = (uint64_t)new_table_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        pdpt_virt = (pdpt_t*)vmm_phys_to_virt((void*)(pml4_virt->entries[pml4_index] & ~0xFFF));
    }
    
    pd_t* pd_virt;
    if (!(pdpt_virt->entries[pdpt_index] & PAGE_PRESENT)) {
        void* new_table_phys = pmm_alloc_page();
        if (!new_table_phys) panic("VMM: Out of memory for PD!", NULL);
        pd_virt = (pd_t*)vmm_phys_to_virt((void*)new_table_phys);
        memset(pd_virt, 0, PAGE_SIZE);
        pdpt_virt->entries[pdpt_index] = (uint64_t)new_table_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        pd_virt = (pd_t*)vmm_phys_to_virt((void*)(pdpt_virt->entries[pdpt_index] & ~0xFFF));
    }
    
    pt_t* pt_virt;
    if (!(pd_virt->entries[pd_index] & PAGE_PRESENT)) {
        void* new_table_phys = pmm_alloc_page();
        if (!new_table_phys) panic("VMM: Out of memory for PT!", NULL);
        pt_virt = (pt_t*)vmm_phys_to_virt((void*)new_table_phys);
        memset(pt_virt, 0, PAGE_SIZE);
        pd_virt->entries[pd_index] = (uint64_t)new_table_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        pt_virt = (pt_t*)vmm_phys_to_virt((void*)(pd_virt->entries[pd_index] & ~0xFFF));
    }
    
    pt_virt->entries[pt_index] = (uint64_t)phys | flags;

    __asm__ __volatile__("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

void vmm_map_page_current(void* virt, void* phys, uint64_t flags) {
    vmm_map_page(vmm_get_current_pml4(), virt, phys, flags);
}

void* vmm_unmap_page(pml4_t* pml4_virt, void* virt) {
    uint64_t virt_addr = (uint64_t)virt;
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    if (!(pml4_virt->entries[pml4_index] & PAGE_PRESENT)) return NULL;
    
    pdpt_t* pdpt_virt = (pdpt_t*)vmm_phys_to_virt(vmm_virt_to_phys((void*)(pml4_virt->entries[pml4_index] & ~0xFFF)));
    if (!(pdpt_virt->entries[pdpt_index] & PAGE_PRESENT)) return NULL;
    
    pd_t* pd_virt = (pd_t*)vmm_phys_to_virt(vmm_virt_to_phys((void*)(pdpt_virt->entries[pdpt_index] & ~0xFFF)));
    if (!(pd_virt->entries[pd_index] & PAGE_PRESENT)) return NULL;

    pt_t* pt_virt = (pt_t*)vmm_phys_to_virt(vmm_virt_to_phys((void*)(pd_virt->entries[pd_index] & ~0xFFF)));
    if (!(pt_virt->entries[pt_index] & PAGE_PRESENT)) return NULL;

    void* phys_addr = (void*)(pt_virt->entries[pt_index] & ~0xFFF);

    pt_virt->entries[pt_index] = 0;
    
    __asm__ __volatile__("invlpg (%0)" :: "r"(virt_addr) : "memory");

    return phys_addr;
}

void* vmm_unmap_page_current(void* virt) {
    return vmm_unmap_page(vmm_get_current_pml4(), virt);
}

void vmm_destroy_address_space(pml4_t* pml4) {
    if (!pml4) return;

    for (int i = 0; i < 256; i++) {
        if (pml4->entries[i] & PAGE_PRESENT) {
            pdpt_t* pdpt = (pdpt_t*)vmm_phys_to_virt((void*)(pml4->entries[i] & ~0xFFF));
            for (int j = 0; j < 512; j++) {
                if (pdpt->entries[j] & PAGE_PRESENT) {
                    pd_t* pd = (pd_t*)vmm_phys_to_virt((void*)(pdpt->entries[j] & ~0xFFF));
                    for (int k = 0; k < 512; k++) {
                        if (pd->entries[k] & PAGE_PRESENT) {
                            pt_t* pt = (pt_t*)vmm_phys_to_virt((void*)(pd->entries[k] & ~0xFFF));
                            pmm_free_page(vmm_virt_to_phys((void*)pt));
                        }
                    }
                    pmm_free_page(vmm_virt_to_phys((void*)pd));
                }
            }
            pmm_free_page(vmm_virt_to_phys((void*)pdpt));
        }
    }
    pmm_free_page(vmm_virt_to_phys((void*)pml4));
}
