# KyroOS Linker Script

ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xFFFFFFFF80000000;
KERNEL_PHYSICAL_BASE = 0x100000;

SECTIONS {
    /* Boot sections at low physical addresses (1MB) */
    . = KERNEL_PHYSICAL_BASE;
    
    .multiboot_header : AT(ADDR(.multiboot_header)) {
        *(.multiboot_header)
    }
    
    .boot_text : AT(ADDR(.boot_text)) {
        *(.boot_text)
    }
    
    .boot_data : AT(ADDR(.boot_data)) {
        *(.boot_data)
    }
    
    .boot_bss (NOLOAD) : AT(ADDR(.boot_bss)) {
        *(.boot_bss)
    }
    
    /* Kernel sections at high virtual addresses */
    . = KERNEL_VIRTUAL_BASE;
    _kernel_start = .;
    
    .text : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) {
        *(.text)
    }
    
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE) {
        *(.rodata)
    }
    
    .data : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) {
        *(.data)
    }
    
    .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) {
        *(.bss)
        *(COMMON)
    }
    
    _kernel_end = .;
    _end = .;
}
