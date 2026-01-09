#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// CPU state pushed onto the stack by the ISR stub
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void isr_handler(struct registers regs);

// IRQ handlers
typedef void (*irq_handler_t)(struct registers);
void register_irq_handler(uint8_t irq, irq_handler_t handler);

static inline void enable_interrupts() {
    __asm__ __volatile__("sti");
}

static inline void disable_interrupts() {
    __asm__ __volatile__("cli");
}

#endif // ISR_H