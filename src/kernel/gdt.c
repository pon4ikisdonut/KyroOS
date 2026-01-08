#include "gdt.h"
#include "tss.h" // For tss_entry_struct

// GDT selectors
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR   0x18
#define USER_DATA_SELECTOR   0x20
#define TSS_SELECTOR         0x28

// External function to load the GDT and segment registers
extern void gdt_flush(uint64_t gdt_ptr_addr, uint16_t code_selector, uint16_t data_selector);

// GDT entries and pointer
#define GDT_ENTRIES 6 // Null, Kernel Code/Data, User Code/Data, TSS
static uint64_t gdt[GDT_ENTRIES];
static struct gdt_ptr_struct gdt_ptr;


// Function to set a GDT entry
static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index] = (uint64_t)limit & 0xFFFF;
    gdt[index] |= ((uint64_t)base & 0xFFFFFF) << 16;
    gdt[index] |= ((uint64_t)access) << 40;
    gdt[index] |= (((uint64_t)limit >> 16) & 0x0F) << 48;
    gdt[index] |= ((uint64_t)gran) << 52;
    gdt[index] |= (((uint64_t)base >> 24) & 0xFF) << 56;
}

// Function to set the TSS entry in the GDT
void gdt_set_tss(void* tss_base) {
    uint64_t base = (uint64_t)tss_base;
    uint32_t limit = sizeof(struct tss_entry_struct);

    // GDT[5] will be the TSS descriptor (low part)
    gdt[5] = (limit & 0xFFFF) | ((base & 0xFFFF) << 16) | (((base >> 16) & 0xFF) << 32) | (0x89ULL << 40) | (((limit >> 16) & 0xF) << 48) | (((base >> 24) & 0xFF) << 56);
    // GDT[6] would be the high part for a 16-byte descriptor, but we'll use a trick
    // to keep it simple. Let's make GDT bigger and use two slots.
    
    // Let's use a proper system segment descriptor structure
    struct gdt_system_entry_bits* tss_desc = (struct gdt_system_entry_bits*)&gdt[5];
    
    tss_desc->limit_low = limit & 0xFFFF;
    tss_desc->base_low = base & 0xFFFF;
    tss_desc->base_middle = (base >> 16) & 0xFF;
    tss_desc->access = 0x89; // Present, DPL=0, Type=9 (64-bit TSS) 
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
    gdt_set_entry(1, 0, 0, 0b10011010, 0b0010); // P=1, DPL=0, S=1, Type=10(code), A=0, R=1, C=0. Gran=1, Sz=1(64bit)
    // Kernel Data Segment (Ring 0)
    gdt_set_entry(2, 0, 0, 0b10010010, 0b0000); // P=1, DPL=0, S=1, Type=2(data), A=0, W=1, E=0.
    // User Code Segment (Ring 3)
    gdt_set_entry(3, 0, 0, 0b11111010, 0b0010); // P=1, DPL=3, S=1, Type=10(code), A=0, R=1, C=0.
    // User Data Segment (Ring 3)
    gdt_set_entry(4, 0, 0, 0b11110010, 0b0000); // P=1, DPL=3, S=1, Type=2(data), A=0, W=1, E=0.

    // TSS descriptor will be set by tss_init() by calling gdt_set_tss()
    
    // Flush the GDT
    gdt_flush((uint64_t)&gdt_ptr, KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);
}

// Assembly stubs for GDT and TSS flush
asm(
"   .global gdt_flush\n"
"gdt_flush:\n"
"   lgdt [rdi]\n"      // Load GDT pointer
"   mov ax, si\n"
"   mov ds, ax\n"
"   mov es, ax\n"
"   mov fs, ax\n"
"   mov gs, ax\n"
"   mov ss, ax\n"
"   push rdx\n"        // Push code selector
"   lea rax, [rip + 1f]\n"
"   push rax\n"
"   retfq\n"           // Far return to new code segment
"1:\n"
"   ret\n"
);

asm(
"   .global tss_flush\n"
"tss_flush:\n"
"   mov ax, " __stringify(TSS_SELECTOR) "\n" // TSS selector
"   ltr ax\n"
"   ret\n"
);