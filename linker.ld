# KyroOS Linker Script
#
ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xFFFFFFFF80000000;

SECTIONS {
    /* The kernel is loaded at 1MB physical, but linked to run at KERNEL_VIRTUAL_BASE. */
    . = KERNEL_VIRTUAL_BASE;
    
    .text : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) {
        *(.multiboot_header)
        _kernel_start = .;
        *(.text)
    }
    
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE) {
        *(.rodata)
    }
    
    .data : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) {
        *(.data)
    }
    
    .bss : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) {
        *(.bss)
    }
    
    _kernel_end = .;
    _end = .;
}
