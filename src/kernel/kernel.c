#include <stdint.h>
#include "kstring.h"
#include <stddef.h> // For NULL

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

// Global page table structures - aligned to 4KB
__attribute__((aligned(4096)))
pml4_t _kernel_pml4; // Now defined directly in C
__attribute__((aligned(4096)))
pdpt_t _kernel_pdpt_low; // For identity mapping low memory
__attribute__((aligned(4096)))
pd_t _kernel_pd_low;     // For identity mapping low memory
__attribute__((aligned(4096)))
pdpt_t _kernel_pdpt_high; // For higher-half kernel mapping
__attribute__((aligned(4096)))
pd_t _kernel_pd_high;     // For higher-half kernel mapping

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

// Function to initialize paging in C
static void setup_paging_c(uint64_t pml4_phys_addr) {
    pml4_t* kernel_pml4 = (pml4_t*)pml4_phys_addr;

    // Clear page tables (PML4 is already cleared by boot.asm)
    memset(&_kernel_pml4, 0, PAGE_SIZE); // Clear our kernel's PML4
    memset(&_kernel_pdpt_low, 0, PAGE_SIZE);
    memset(&_kernel_pd_low, 0, PAGE_SIZE);
    memset(&_kernel_pdpt_high, 0, PAGE_SIZE);
    memset(&_kernel_pd_high, 0, PAGE_SIZE);

    // Identity map first 1GB
    for (int i = 0; i < 512; i++) { // 512 * 2MB = 1GB
        _kernel_pd_low.entries[i] = (i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER | (1 << 7); // 2MB page
    }
    _kernel_pdpt_low.entries[0] = (uint64_t)&_kernel_pdpt_low | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    _kernel_pml4.entries[0] = (uint64_t)&_kernel_pdpt_low | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // Higher-half kernel mapping (example: 512MB starting at KERNEL_PHYSICAL_BASE)
    for (int i = 0; i < 256; i++) { // 256 * 2MB = 512MB
        _kernel_pd_high.entries[i] = (0x100000 + i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER | (1 << 7); // 2MB page
    }
    _kernel_pdpt_high.entries[511] = (uint64_t)&_kernel_pd_high | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    _kernel_pml4.entries[511] = (uint64_t)&_kernel_pdpt_high | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // Reload CR3 with our new PML4 (since boot.asm was using a basic one)
    __asm__ __volatile__("mov %0, %%cr3" : : "r"((uint64_t)&_kernel_pml4));
    klog(LOG_INFO, "Paging setup in C complete.");
}


void kmain_x64(uint32_t multiboot_magic, uint32_t multiboot_info_addr, uint64_t pml4_phys_addr) {
    
    // Initialize framebuffer first, so we can log messages.
    fb_init(multiboot_info_addr);
    log_init();

    klog(LOG_INFO, "KyroOS 26.01.002 \"Helium\""); // Updated version string
    klog(LOG_INFO, "---------------------------------");

    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        panic("Invalid Multiboot magic number!", NULL);
        return;
    }
    klog(LOG_INFO, "Multiboot2 magic: OK");

    // Setup paging in C (full setup, building on minimal ASM setup)
    setup_paging_c((uint64_t)&_kernel_pml4); // Use our locally defined PML4

    // Initialize core kernel systems
    gdt_init();
    klog(LOG_INFO, "GDT initialized.");
    tss_init();
    klog(LOG_INFO, "TSS initialized.");
    idt_init();
    klog(LOG_INFO, "IDT initialized.");
    pmm_init(multiboot_info_addr);
    vmm_init(); // VMM now relies on _kernel_pml4
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
        __asm__ __volatile__ ("hlt");
    }
}
