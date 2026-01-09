;
; KyroOS Context Switching
;
section .text
global thread_switch

; void thread_switch(thread_t* old_thread, thread_t* new_thread);
; rdi = old_thread
; rsi = new_thread
thread_switch:
    ; Save old thread's context
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Save old stack pointer
    mov [rdi + 0], rsp ; old_thread->rsp = rsp

    ; Restore new thread's context
    mov rsp, [rsi + 0] ; rsp = new_thread->rsp
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
