;
; KyroOS x86_64 Boot Assembly
; This file contains the initial 32-bit boot code, transitions to 64-bit long mode.
; GDT and paging setup is now done in C.
;

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

    ; End tag
    dw 0, 0, 8
header_end:

section .boot_data
; Local GDT for initial long mode transition
align 8
local_boot_gdt:
    dq 0 ; Null segment
    dq 0x00AF9A000000FFFF ; 64-bit Code Segment (Selector 0x08)
    dq 0x00CF92000000FFFF ; 64-bit Data Segment (Selector 0x10)
.end:
local_boot_gdt_ptr:
    dw local_boot_gdt.end - local_boot_gdt - 1
    dq local_boot_gdt

section .boot_bss nobits
align 4096
kernel_stack: resb 8192
kernel_stack_top:

; Local dummy PML4 for initial long mode transition
align 4096
local_dummy_pml4: resb 4096

global _start
extern kmain_x64

section .boot_text
bits 32

_start:
    ; Clean up segment registers
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Clear dummy PML4
    mov edi, local_dummy_pml4
    mov ecx, 1024 ; 512 entries * 8 bytes / 4 bytes per dword = 1024 dwords
    xor eax, eax
    rep stosd
    
    ; --- Enable Long Mode ---
    mov eax, cr4
    or eax, 1 << 5 ; PAE
    mov cr4, eax
    
    mov eax, local_dummy_pml4
    mov cr3, eax

    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or eax, 1 << 8 ; LME
    wrmsr

    mov eax, cr0
    or eax, (1 << 31) | (1 << 0) ; PG and PE
    mov cr0, eax

    lgdt [local_boot_gdt_ptr] ; Load temporary GDT

    ; JUMP TO LONG MODE CODE
    mov ecx, local_dummy_pml4
    jmp 0x08:long_mode_entry

bits 64
long_mode_entry:
    ; Reload segment registers for 64-bit mode.
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    xor rax, rax
    mov fs, rax
    mov gs, rax

    ; Set up 64-bit stack
    mov rsp, kernel_stack_top

    ; Prepare arguments for kmain_x64
    ; RDI = multiboot magic (from EAX)
    ; RSI = multiboot info (from EBX)
    ; RDX = PML4 physical address (from ECX)
    mov rdx, rcx
    mov rsi, rbx
    mov rdi, rax
    
    ; Use absolute addressing to call kmain_x64 in high memory
    mov rax, kmain_x64
    call rax

hang_loop:
    cli
    hlt
    jmp hang_loop

section .note.GNU-stack noalloc noexec nowrite progbits
