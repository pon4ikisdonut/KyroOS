#include "isr.h"
#include "log.h"
#include "port_io.h"
#include "scheduler.h"

// Array of IRQ handlers
static irq_handler_t irq_handlers[16] = {0};

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero", "Debug", "Non-Maskable Interrupt", "Breakpoint", "Overflow",
    "Bound Range Exceeded", "Invalid Opcode", "Device Not Available", "Double Fault",
    "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
    "x87 FPU Error", "Alignment Check", "Machine Check", "SIMD Floating-Point Exception",
    "Virtualization Exception", "Control Protection Exception", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

void isr_handler(struct registers regs) {
    if (regs.int_no < 32) { // CPU Exceptions
        panic(exception_messages[regs.int_no], &regs);
    } else if (regs.int_no >= 32 && regs.int_no <= 47) { // IRQs
        uint8_t irq_num = regs.int_no - 32;

        if (irq_handlers[irq_num] != 0) {
            irq_handlers[irq_num](regs);
        }

        // Send EOI (End Of Interrupt) to PIC before scheduling
        if (irq_num >= 8) {
            outb(0xA0, 0x20); // Send EOI to slave
        }
        outb(0x20, 0x20); // Send EOI to master
        
        // Call the scheduler on timer interrupt
        if (irq_num == 0) {
            schedule();
        }
    }
}

void register_irq_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}
