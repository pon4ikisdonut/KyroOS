#include "gdt.h"
#include "tss.h" // For tss_entry_struct

// GDT selectors
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR   0x18
#define USER_DATA_SELECTOR   0x20
#define TSS_SELECTOR         0x28

// External assembly function to load the GDT and segment registers
extern void gdt_flush(uint64_t gdt_ptr_addr);

// GDT entries and pointer
#define GDT_ENTRIES 7 // Null, Kernel Code/Data, User Code/Data, TSS (2 entries for 16 bytes)
static uint64_t gdt[GDT_ENTRIES];
static struct gdt_ptr_struct gdt_ptr;


// Function to set a GDT entry
static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index] = (uint64_t)(limit & 0xFFFF) |
                 (((uint64_t)base & 0xFFFFFF) << 16) |
                 (((uint64_t)access) << 40) |
                 (((uint64_t)(limit >> 16) & 0x0F) << 48) |
                 (((uint64_t)gran) << 52) |
                 (((uint64_t)(base >> 24) & 0xFF) << 56);
}

// Function to set the TSS entry in the GDT
void gdt_set_tss(void* tss_base) {
    uint64_t base = (uint64_t)tss_base;
    uint32_t limit = sizeof(struct tss_entry_struct);

    // Let's use a proper system segment descriptor structure
    struct gdt_system_entry_bits* tss_desc = (struct gdt_system_entry_bits*)&gdt[5];
    
    tss_desc->limit_low = limit & 0xFFFF;
    tss_desc->base_low = base & 0xFFFF;
    tss_desc->base_middle = (base >> 16) & 0xFF;
    tss_desc->access = 0x89; // Present, DPL=0, Type=9 (64-bit TSS Available)
    tss_desc->granularity = (limit >> 16) & 0x0F;
    tss_desc->base_high = (base >> 24) & 0xFF;
    tss_desc->base_upper = (base >> 32) & 0xFFFFFFFF;
    tss_desc->reserved = 0;
}

void gdt_init() {
    gdt_ptr.limit = (sizeof(uint64_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint64_t)&gdt;

    // Null segment
    gdt_set_entry(0, 0, 0, 0, 0);
    // Kernel Code Segment (Ring 0)
    gdt_set_entry(1, 0, 0, 0x9A, 0x20);
    // Kernel Data Segment (Ring 0)
    gdt_set_entry(2, 0, 0, 0x92, 0x00);
    // User Code Segment (Ring 3)
    gdt_set_entry(3, 0, 0, 0xFA, 0x20);
    // User Data Segment (Ring 3)
    gdt_set_entry(4, 0, 0, 0xF2, 0x00);

    // TSS descriptor will be set by tss_init() by calling gdt_set_tss()
    
    // Flush the GDT
    gdt_flush((uint64_t)&gdt_ptr);
}
