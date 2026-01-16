#include "isr.h"
#include "log.h"
#include "port_io.h"
#include "scheduler.h"

extern void syscall_handler(struct registers *regs); // Declare syscall_handler here

// Array of IRQ handlers
static irq_handler_t irq_handlers[16] = {0};

// Exception messages
static const char *exception_messages[] = {"Division By Zero",
                                           "Debug",
                                           "Non-Maskable Interrupt",
                                           "Breakpoint",
                                           "Overflow",
                                           "Bound Range Exceeded",
                                           "Invalid Opcode",
                                           "Device Not Available",
                                           "Double Fault",
                                           "Coprocessor Segment Overrun",
                                           "Invalid TSS",
                                           "Segment Not Present",
                                           "Stack-Segment Fault",
                                           "General Protection Fault",
                                           "Page Fault",
                                           "Reserved",
                                           "x87 FPU Error",
                                           "Alignment Check",
                                           "Machine Check",
                                           "SIMD Floating-Point Exception",
                                           "Virtualization Exception",
                                           "Control Protection Exception",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved"};

static uint64_t ticks = 0;

uint64_t timer_get_ticks() { return ticks; }

void timer_init(uint32_t frequency) {
  uint32_t divisor = 1193180 / frequency;

  // Send the command byte.
  outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8_t l = (uint8_t)(divisor & 0xFF);
  uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);
}

void isr_handler(struct registers *regs) {
  if (regs->int_no < 32) { // CPU Exceptions
    panic(exception_messages[regs->int_no], regs);
  } else if (regs->int_no >= 32 && regs->int_no <= 47) { // IRQs
    uint8_t irq_num = regs->int_no - 32;

    if (irq_num == 0) {
      ticks++;
    }

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
  } else if (regs->int_no == 128) { // Syscall interrupt (0x80)
      syscall_handler(regs);
  }
}

void register_irq_handler(uint8_t irq, irq_handler_t handler) {
  if (irq < 16) {
    irq_handlers[irq] = handler;
  }
}
