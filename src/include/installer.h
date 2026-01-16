#ifndef INSTALLER_H
#define INSTALLER_H

#include "ide.h" // For IDE IOCTL requests and structs
#include <stdint.h>
#include <stddef.h> // For size_t

// MBR Partition Entry Structure
typedef struct {
    uint8_t boot_flag;      // 0x80 for bootable, 0x00 otherwise
    uint8_t starting_chs[3]; // Cylinder, Head, Sector (legacy)
    uint8_t type_code;      // Partition type (e.g., 0x0C for FAT32 LBA)
    uint8_t ending_chs[3];   // Cylinder, Head, Sector (legacy)
    uint32_t starting_lba;   // Starting LBA address
    uint32_t size_in_sectors; // Size of partition in sectors
} __attribute__((packed)) mbr_partition_entry_t;

// Master Boot Record Structure
typedef struct {
    uint8_t boot_code[440]; // Bootstrap code area
    uint32_t signature;     // Optional disk signature
    uint16_t reserved;      // Reserved (usually 0x0000)
    mbr_partition_entry_t partitions[4]; // Four primary partition entries
    uint16_t boot_signature; // MBR signature (0xAA55)
} __attribute__((packed)) mbr_t;

// Function declarations for the installer components
void installer_disk_detection();
void installer_partition_selection();
void installer_filesystem_formatting();
void installer_base_system_installation();
void installer_install_initial_packages();
void installer_install_bootloader();
void installer_create_initial_user();
void installer_basic_configuration();

#endif // INSTALLER_H
