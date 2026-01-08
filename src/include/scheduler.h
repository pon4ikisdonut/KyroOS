#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "thread.h"

void scheduler_init();
void schedule();
void scheduler_add_thread(thread_t* thread);
thread_t* get_current_thread();

#endif // SCHEDULER_H
