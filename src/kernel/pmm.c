#include "pmm.h"
#include "log.h"
#include <stddef.h> // for NULL

// Bitmap for physical memory management
static uint8_t* pmm_bitmap = NULL;
static uint64_t pmm_total_pages = 0;
static uint64_t pmm_last_free_page = 0;

// External symbols from linker script to know kernel's location
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

// Set a bit in the bitmap
static void pmm_bitmap_set(uint64_t page_index) {
    pmm_bitmap[page_index / 8] |= (1 << (page_index % 8));
}

// Unset a bit in the bitmap
static void pmm_bitmap_unset(uint64_t page_index) {
    pmm_bitmap[page_index / 8] &= ~(1 << (page_index % 8));
}

// Test a bit in the bitmap
static int pmm_bitmap_test(uint64_t page_index) {
    return pmm_bitmap[page_index / 8] & (1 << (page_index % 8));
}

void pmm_init(uint32_t multiboot_info_addr) {
    struct multiboot_tag_mmap* mmap_tag = (struct multiboot_tag_mmap*)find_multiboot_tag(multiboot_info_addr, MULTIBOOT_TAG_TYPE_MMAP);

    if (!mmap_tag) {
        panic("PMM: Failed to find memory map in Multiboot info!", NULL);
        return;
    }

    // 1. Find total memory size to determine bitmap size
    uint64_t highest_addr = 0;
    for (struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)(mmap_tag + 1);
         (uint8_t*)entry < (uint8_t*)mmap_tag + mmap_tag->size;
         entry = (struct multiboot_mmap_entry*)((uint64_t)entry + mmap_tag->entry_size)) {
        if (entry->addr + entry->len > highest_addr) {
            highest_addr = entry->addr + entry->len;
        }
    }
    pmm_total_pages = highest_addr / PAGE_SIZE;
    uint64_t bitmap_size = pmm_total_pages / 8;
    if (bitmap_size == 0) bitmap_size = 1;

    // 2. Find a place for the bitmap
    for (struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)(mmap_tag + 1);
         (uint8_t*)entry < (uint8_t*)mmap_tag + mmap_tag->size;
         entry = (struct multiboot_mmap_entry*)((uint64_t)entry + mmap_tag->entry_size)) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len >= bitmap_size) {
            // Found a spot, but ensure it's above 1MB to not conflict with legacy hardware/boot code
            if (entry->addr >= 0x100000) {
                 pmm_bitmap = (uint8_t*)entry->addr;
                 break;
            }
        }
    }
    if (pmm_bitmap == NULL) {
        panic("PMM: Could not find a suitable location for the memory bitmap!", NULL);
        return;
    }

    // 3. Initialize bitmap: mark all memory as used
    for (uint64_t i = 0; i < bitmap_size; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    // 4. Mark available memory regions as free
    for (struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)(mmap_tag + 1);
         (uint8_t*)entry < (uint8_t*)mmap_tag + mmap_tag->size;
         entry = (struct multiboot_mmap_entry*)((uint64_t)entry + mmap_tag->entry_size)) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            for (uint64_t i = 0; i < entry->len / PAGE_SIZE; i++) {
                pmm_bitmap_unset((entry->addr / PAGE_SIZE) + i);
            }
        }
    }

    // 5. Mark kernel and bitmap memory as used
    uint64_t kernel_start_page = (uint64_t)_kernel_start / PAGE_SIZE;
    uint64_t kernel_end_page = ((uint64_t)_kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = kernel_start_page; i < kernel_end_page; i++) {
        pmm_bitmap_set(i);
    }

    uint64_t bitmap_start_page = (uint64_t)pmm_bitmap / PAGE_SIZE;
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        pmm_bitmap_set(bitmap_start_page + i);
    }

    klog(LOG_INFO, "PMM initialized.");
}

void* pmm_alloc_page() {
    for (uint64_t i = pmm_last_free_page; i < pmm_total_pages; i++) {
        if (!pmm_bitmap_test(i)) {
            pmm_bitmap_set(i);
            pmm_last_free_page = i + 1;
            return (void*)(i * PAGE_SIZE);
        }
    }
    // If we wrapped around and still couldn't find a page
    if (pmm_last_free_page > 0) {
        for (uint64_t i = 0; i < pmm_last_free_page; i++) {
            if (!pmm_bitmap_test(i)) {
                pmm_bitmap_set(i);
                pmm_last_free_page = i + 1;
                return (void*)(i * PAGE_SIZE);
            }
        }
    }

    klog(LOG_WARN, "PMM: Out of physical memory!");
    return NULL;
}

void pmm_free_page(void* p) {
    uint64_t page_index = (uint64_t)p / PAGE_SIZE;
    if (page_index < pmm_total_pages) {
        pmm_bitmap_unset(page_index);
        if (page_index < pmm_last_free_page) {
            pmm_last_free_page = page_index;
        }
    } else {
        klog(LOG_WARN, "PMM: Attempted to free an invalid physical page.");
    }
}
