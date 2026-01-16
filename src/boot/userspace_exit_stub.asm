; src/boot/userspace_exit_stub.asm
; This stub is executed in kernel mode after a context switch
; It prepares the stack for an IRETQ to transition to userspace.

global userspace_exit_stub

section .text
userspace_exit_stub:
    ; The stack currently contains the registers saved by thread_switch,
    ; followed by the IRETQ frame (SS, RSP, RFLAGS, CS, RIP) pushed by thread_create_userspace.

    ; We need to move the IRETQ frame from the kernel stack
    ; to the correct position for IRETQ.
    ; RSP should point directly to the saved SS value.

    ; Let's assume the IRETQ frame is structured on the kernel stack as:
    ; ...
    ; RBP
    ; RBX
    ; R12
    ; R13
    ; R14
    ; R15
    ; CS (userspace)
    ; RIP (userspace)
    ; RFLAGS
    ; RSP (userspace)
    ; SS (userspace)

    ; If thread_create_userspace pushes the IRETQ frame and then the fake callee-saved registers,
    ; then thread_switch pops the fake registers and 'ret's to this stub.

    ; This stub receives control in kernel mode.
    ; The kernel stack (RSP) now contains the IRETQ frame (SS, RSP, RFLAGS, CS, RIP)
    ; pushed by thread_create_userspace.

    ; Simply execute IRETQ.
    ; This will pop SS, RSP, RFLAGS, CS, RIP from the *current* kernel stack (RSP)
    ; and switch to userspace.

    iretq
