#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

// Tag types
#define MULTIBOOT_TAG_TYPE_END              0
#define MULTIBOOT_TAG_TYPE_CMDLINE          1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE           3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO    4
#define MULTIBOOT_TAG_TYPE_BOOTDEV          5
#define MULTIBOOT_TAG_TYPE_MMAP             6
#define MULTIBOOT_TAG_TYPE_VBE              7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER      8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS     9
#define MULTIBOOT_TAG_TYPE_APM              10
#define MULTIBOOT_TAG_TYPE_EFI32_IH         11
#define MULTIBOOT_TAG_TYPE_EFI64_IH         12
#define MULTIBOOT_TAG_TYPE_SMBIOS           13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD         14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW         15
#define MULTIBOOT_TAG_TYPE_NETWORK          16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP         17
#define MULTIBOOT_TAG_TYPE_EFI_BS           18
#define MULTIBOOT_TAG_TYPE_EFI32_RT_POINTER 19
#define MULTIBOOT_TAG_TYPE_EFI64_RT_POINTER 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR   21

// Base tag structure
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

// Memory map tag
struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // Followed by a variable number of multiboot_mmap_entry
};

// Memory map entry
struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE          1
#define MULTIBOOT_MEMORY_RESERVED           2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE   3
#define MULTIBOOT_MEMORY_NVS                4
#define MULTIBOOT_MEMORY_BADRAM             5
    uint32_t type;
    uint32_t zero; // Should be 0
} __attribute__((packed));

// Helper function to find a multiboot tag
static inline struct multiboot_tag* find_multiboot_tag(uint32_t info_addr, uint16_t type) {
    struct multiboot_tag *tag;
    // The info structure starts with total size and reserved field
    for (tag = (struct multiboot_tag *)(uint64_t)(info_addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == type) {
            return tag;
        }
    }
    return 0;
}

#endif // MULTIBOOT2_H
