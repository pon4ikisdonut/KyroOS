#include <stdint.h>
#include <string.h>

#include "gdt.h"
#include "idt.h"
#include "log.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "tss.h"
#include "elf.h"
#include "userspace.h"
#include "syscall.h"
#include "multiboot2.h"
#include "fb.h"
#include "event.h"
#include "keyboard.h"
#include "mouse.h"

// Helper function to find a multiboot module tag by its command line string
static struct multiboot_tag_module* find_module_tag(uint32_t info_addr, const char* name) {
    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*)(uint64_t)(info_addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module* mod_tag = (struct multiboot_tag_module*)tag;
            const char* mod_name = (const char*)mod_tag->cmdline;
            
            int i = 0;
            while (name[i] != '\0' && mod_name[i] != '\0' && name[i] == mod_name[i]) {
                i++;
            }
            if (name[i] == '\0' && mod_name[i] == '\0') {
                return mod_tag;
            }
        }
    }
    return NULL;
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

    // Initialize core kernel systems
    gdt_init();
    tss_init();
    idt_init();
    pmm_init(multiboot_info_addr);
    vmm_init();
    heap_init();
    syscall_init();
    event_init();
    keyboard_init();
    mouse_init();
    
    // Find the game program module
    struct multiboot_tag_module* game_module = find_module_tag(multiboot_info_addr, "/boot/game.elf");
    if (!game_module) {
        panic("Could not find game.elf module!", NULL);
        return;
    }
    klog(LOG_INFO, "Found game.elf module.");

    // Load the game program
    uint64_t entry_point = elf_load((const uint8_t*)((uint64_t)game_module->mod_start));

    if (entry_point == 0) {
        panic("Failed to load game.elf!", NULL);
        return;
    }
    
    // Enable interrupts before jumping to userspace
    enable_interrupts();
    klog(LOG_INFO, "Interrupts enabled.");

    // Enter userspace
    enter_userspace(entry_point);

    // We should never get here
    klog(LOG_ERROR, "Failed to enter userspace.");
    for (;;) {
        asm volatile ("hlt");
    }
}
