#include <stdint.h>
#include <string.h>

#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "log.h"
#include "port_io.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "thread.h"
#include "scheduler.h"
#include "syscall.h"
#include "vfs.h"
#include "kyrofs.h"
#include "deviceman.h"
#include "pci.h"
#include "null_pci_driver.h"
#include "net.h"
#include "e1000.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "fb.h"
#include "gui.h"
#include "mouse.h"
#include "audio.h"
#include "ac97.h"
#include "math.h" // Our own math header

#define PI 3.1415926535

void timer_handler(struct registers regs) {
    // Empty for now
}

void init_timer(uint32_t frequency) {
    register_irq_handler(0, timer_handler);
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void kmain_x64(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    
    // Initialize framebuffer first, so we can log messages.
    fb_init(multiboot_info_addr);
    log_init();

    klog(LOG_INFO, "KyroOS 26.01 \"Helium\" alpha");
    klog(LOG_INFO, "---------------------------------");

    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        panic("Invalid Multiboot magic number!", NULL);
        return;
    }

    // Initialize core systems
    gdt_init();
    idt_init();
    pmm_init(multiboot_info_addr);
    vmm_init();
    heap_init();
    thread_init();
    syscall_init();
    vfs_init();
    kyrofs_init();
    
    // Initialize Device Driver Framework, Networking, and Audio
    deviceman_init();
    null_pci_driver_init();
    net_init();
    e1000_driver_init();
    arp_init();
    ip_init();
    icmp_init();
    audio_init();
    ac97_driver_init();
    
    pci_check_all_buses();
    deviceman_probe_devices();

    // Start timer and interrupts
    init_timer(100);
    enable_interrupts();
    klog(LOG_INFO, "Interrupts enabled. Kernel setup complete.");
    
    // Test Audio
    klog(LOG_INFO, "Audio Test: Generating a 440Hz sine wave...");
    size_t sample_rate = 48000;
    size_t duration = 2; // seconds
    size_t num_samples = sample_rate * duration;
    int16_t* audio_buffer = (int16_t*)kmalloc(num_samples * sizeof(int16_t));
    if (audio_buffer) {
        for (size_t i = 0; i < num_samples; i++) {
            double time = (double)i / sample_rate;
            // A simple sine wave at 440 Hz (A4 note)
            audio_buffer[i] = (int16_t)(32760.0 * sin(2 * PI * 440.0 * time));
        }
        klog(LOG_INFO, "Playing sound...");
        audio_play(audio_buffer, num_samples * sizeof(int16_t));
        kfree(audio_buffer);
    } else {
        klog(LOG_ERROR, "Failed to allocate audio buffer.");
    }

    // Idle loop
    for (;;) {
        asm volatile ("hlt");
    }
}