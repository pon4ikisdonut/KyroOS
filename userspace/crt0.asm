; Userspace Entry Point Stub
bits 64
extern main
extern _bss_start, _bss_end
global _start

section .text
_start:
    ; Clear BSS section
    xor rax, rax
    mov rdi, _bss_start
    mov rcx, _bss_end
    sub rcx, rdi
    rep stosb

    ; Align stack and call main
    xor rbp, rbp
    push rax ; Align stack to 16-bytes for ABI
    call main
    
    ; Exit syscall (assumed 1 for KyroOS)
    mov rax, 1
    mov rdi, 0
    syscall

    ; Should not reach here
.hang:
    pause
    jmp .hang
