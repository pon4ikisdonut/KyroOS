#include "vmm.h"
#include "pmm.h"
#include "log.h"
#include <stddef.h> // for NULL

// Kernel's top-level page map (PML4)
// This is created in boot.asm and we get a pointer to it.
extern pml4_t pml4[];
static pml4_t* kernel_pml4 = (pml4_t*)pml4;

void vmm_init() {
    // The bootloader has already set up identity mapping for the first 1GB
    // and mapped the kernel to the higher half.
    // We can now take control of the page tables.
    
    // For now, no extra initialization is needed as bootloader did the heavy lifting.
    // Future work: clean up the temporary boot page tables.
    
    klog(LOG_INFO, "VMM initialized.");
}

// Helper to get the physical address of the PML4
static inline uint64_t get_pml4_phys() {
    uint64_t pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_phys));
    return pml4_phys;
}

void vmm_map_page(void* virt, void* phys, uint64_t flags) {
    uint64_t virt_addr = (uint64_t)virt;
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)get_pml4_phys();
    
    // PDPT
    pdpt_t* pdpt;
    if (!(pml4->entries[pml4_index] & PAGE_PRESENT)) {
        pdpt = pmm_alloc_page();
        if (!pdpt) panic("VMM: Out of memory for PDPT!", NULL);
        for(int i = 0; i < 512; i++) pdpt->entries[i] = 0;
        pml4->entries[pml4_index] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pdpt = (pdpt_t*)(pml4->entries[pml4_index] & ~0xFFF);
    }
    
    // Page Directory
    pd_t* pd;
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        pd = pmm_alloc_page();
        if (!pd) panic("VMM: Out of memory for PD!", NULL);
        for(int i = 0; i < 512; i++) pd->entries[i] = 0;
        pdpt->entries[pdpt_index] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pd = (pd_t*)(pdpt->entries[pdpt_index] & ~0xFFF);
    }
    
    // Page Table
    pt_t* pt;
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        pt = pmm_alloc_page();
        if (!pt) panic("VMM: Out of memory for PT!", NULL);
        for(int i = 0; i < 512; i++) pt->entries[i] = 0;
        pd->entries[pd_index] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pt = (pt_t*)(pd->entries[pd_index] & ~0xFFF);
    }
    
    // Map the page
    pt->entries[pt_index] = (uint64_t)phys | flags;

    // Invalidate the TLB entry for this address
    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

void vmm_unmap_page(void* virt) {
    // This is a simplified version. A real implementation would need to free
    // page tables if they become empty.
    
    uint64_t virt_addr = (uint64_t)virt;
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)get_pml4_phys();
    if (!(pml4->entries[pml4_index] & PAGE_PRESENT)) return;
    
    pdpt_t* pdpt = (pdpt_t*)(pml4->entries[pml4_index] & ~0xFFF);
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) return;
    
    pd_t* pd = (pd_t*)(pdpt->entries[pdpt_index] & ~0xFFF);
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) return;

    pt_t* pt = (pt_t*)(pd->entries[pd_index] & ~0xFFF);
    if (!(pt->entries[pt_index] & PAGE_PRESENT)) return;

    // Unmap page
    pt->entries[pt_index] = 0;
    
    // Invalidate TLB
    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}