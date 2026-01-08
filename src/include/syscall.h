#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

// Syscall numbers
#define SYS_EXIT  0
#define SYS_WRITE 1
#define SYS_OPEN  2
#define SYS_READ  3
#define SYS_STAT  4
#define SYS_GFX_GET_FB_INFO 5
#define SYS_INPUT_POLL_EVENT 6

void syscall_init();
void syscall_handler(struct registers* regs);

#endif // SYSCALL_H
