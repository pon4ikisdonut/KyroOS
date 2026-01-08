#include "event.h"
#include "log.h"
#include "isr.h" // For disable/enable interrupts

#define EVENT_QUEUE_SIZE 256

static event_t event_queue[EVENT_QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
static int event_count = 0;

void event_init() {
    queue_head = 0;
    queue_tail = 0;
    event_count = 0;
    klog(LOG_INFO, "Event queue initialized.");
}

void event_push(event_t event) {
    disable_interrupts();
    if (event_count < EVENT_QUEUE_SIZE) {
        event_queue[queue_tail] = event;
        queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
        event_count++;
    } else {
        klog(LOG_WARN, "Event queue full, event dropped.");
    }
    enable_interrupts();
}

int event_pop(event_t* event) {
    disable_interrupts();
    if (event_count > 0) {
        *event = event_queue[queue_head];
        queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
        event_count--;
        enable_interrupts();
        return 1; // Success
    }
    enable_interrupts();
    return 0; // Queue was empty
}
