;
; KyroOS x86_64 Boot Assembly
; This file contains the initial 32-bit boot code, transitions to 64-bit long mode,
; and sets up paging for a higher-half kernel.
;
default rel

KERNEL_VIRTUAL_BASE equ 0xFFFFFFFF80000000
KERNEL_PHYSICAL_BASE equ 0x100000 ; 1MB

section .multiboot_header
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    
    ; Framebuffer tag
    dw 5
    dw 0
    dd 20
    dd 1024  ; width
    dd 768   ; height
    dd 32    ; bpp

    ; Address tag to tell GRUB to load us at KERNEL_PHYSICAL_BASE
    dw 2
    dw 0
    dd 24
    dd KERNEL_PHYSICAL_BASE
    dd KERNEL_VIRTUAL_BASE
    dd _kernel_end - _kernel_start
    dd _bss_end - _bss_start
    ; End tag
    dw 0, 0, 8
header_end:

section .data
align 8
boot_gdt:
    dq 0
.code_segment: dq 0x00AF9A000000FFFF
.data_segment: dq 0x00CF92000000FFFF
.end:
boot_gdt_ptr:
    dw boot_gdt.end - boot_gdt - 1
    dq boot_gdt

section .bss
align 4096
pml4: resq 512
pdpt_low: resq 512
pd_low: resq 512
pdpt_high: resq 512
pd_high: resq 512

global _start, _kernel_start, _kernel_end, _bss_start, _bss_end
extern kmain_x64

section .text
bits 32

_start:
    ; Clean up segment registers
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; --- Setup Paging for Higher-Half Kernel ---
    ; Clear paging structures
    mov edi, pml4
    mov ecx, 4096 * 5 / 4 ; 5 pages (pml4, pdpt_low, pd_low, pdpt_high, pd_high)
    xor eax, eax
    rep stosd

    ; Map first 1GB identity
    mov edi, pd_low
    mov ecx, 512
    mov eax, 0x83 ; Present, R/W, 2MB page
    .map_low_1gb:
        mov [edi], eax
        add eax, 0x200000 ; Next 2MB frame
        add edi, 8
        loop .map_low_1gb

    ; Link paging structures for low memory
    mov edi, pml4
    mov edx, pdpt_low + 3
    mov [edi], edx ; pml4[0] -> pdpt_low
    mov edi, pdpt_low
    mov edx, pd_low + 3
    mov [edi], edx ; pdpt_low[0] -> pd_low

    ; Map kernel to higher half
    mov edi, pdpt_high
    mov edx, pd_high + 3
    mov [edi + 511*8], edx ; pdpt_high[511] -> pd_high
    
    ; The kernel is small, it will fit in one PD.
    ; We map from KERNEL_PHYSICAL_BASE to KERNEL_VIRTUAL_BASE
    ; This requires mapping 2MB pages.
    mov edi, pd_high
    mov ecx, 256 ; Map 512MB for now
    mov eax, KERNEL_PHYSICAL_BASE | 0x83
    .map_kernel:
        mov [edi], eax
        add eax, 0x200000
        add edi, 8
        loop .map_kernel
        
    ; Link paging structures for high memory
    mov edi, pml4
    mov edx, pdpt_high + 3
    mov [edi + 511*8], edx ; pml4[511] -> pdpt_high


    ; --- Enable Long Mode ---
    mov eax, cr4
    or eax, 1 << 5 ; PAE
    mov cr4, eax

    mov eax, pml4
    mov cr3, eax

    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or eax, 1 << 8 ; LME
    wrmsr

    mov eax, cr0
    or eax, (1 << 31) | (1 << 0) ; PG and PE
    mov cr0, eax

    lgdt [boot_gdt_ptr]
    jmp 0x08:long_mode_start

bits 64
long_mode_start:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    xor rax, rax
    mov fs, rax
    mov gs, rax

    mov rsp, kernel_stack_top

    mov edi, eax ; Multiboot magic
    mov rsi, ebx ; Multiboot info
    call kmain_x64

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 4096
kernel_stack: resb 8192
kernel_stack_top:

_bss_start:
    ; This is filled by linker
_bss_end: