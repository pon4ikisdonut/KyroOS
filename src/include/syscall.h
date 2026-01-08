#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

// Syscall numbers
#define SYS_EXIT 0
#define SYS_WRITE 1

void syscall_init();
void syscall_handler(struct registers* regs);

#endif // SYSCALL_H
