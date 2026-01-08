#include "elf.h"
#include "log.h"
#include "vmm.h"
#include "pmm.h"
#include "heap.h" // For kmalloc
#include <string.h> // For memcpy, memset

// For now, ELF loading will happen into the current address space.
// A proper implementation would create and switch to a new address space.

int elf_load(const uint8_t* elf_data) {
    const Elf64_Ehdr* header = (const Elf64_Ehdr*)elf_data;

    // 1. Validate ELF Header
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3) {
        klog(LOG_ERROR, "ELF: Invalid magic number.");
        return 0; // Failure
    }
    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        klog(LOG_ERROR, "ELF: Not a 64-bit executable.");
        return 0;
    }
    if (header->e_type != ET_EXEC) {
        klog(LOG_ERROR, "ELF: Not an executable file.");
        return 0;
    }

    klog(LOG_INFO, "ELF: Valid header found. Loading segments...");

    // 2. Iterate through Program Headers
    for (int i = 0; i < header->e_phnum; i++) {
        const Elf64_Phdr* p_header = (const Elf64_Phdr*)(elf_data + header->e_phoff + (i * header->e_phentsize));
        
        if (p_header->p_type == PT_LOAD) {
            // This is a loadable segment
            klog(LOG_INFO, "ELF: Loading segment");
            
            // For now, we assume the virtual address is in userspace and not conflicting.
            // We need to allocate physical memory and map it.
            uint64_t vaddr = p_header->p_vaddr;
            uint64_t mem_size = p_header->p_memsz;
            uint64_t file_size = p_header->p_filesz;
            
            for (uint64_t j = 0; j < mem_size; j += PAGE_SIZE) {
                void* phys_page = pmm_alloc_page();
                if (!phys_page) {
                    klog(LOG_ERROR, "ELF: Out of physical memory to load segment.");
                    return 0;
                }
                // Map the page with user-accessible flags
                vmm_map_page((void*)(vaddr + j), phys_page, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            }

            // Copy data from ELF file to the new memory
            memcpy((void*)vaddr, (void*)(elf_data + p_header->p_offset), file_size);
            
            // Zero out the BSS part of the segment
            if (mem_size > file_size) {
                memset((void*)(vaddr + file_size), 0, mem_size - file_size);
            }
        }
    }

    klog(LOG_INFO, "ELF: Segments loaded.");
    return header->e_entry;
}
