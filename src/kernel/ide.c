#include "ide.h"
#include "deviceman.h"
#include "pci.h"
#include "log.h"
#include "heap.h"
#include "port_io.h"
#include "kstring.h"
#include "vfs.h"   // For vfs_node_t, vfs_root, vfs_finddir, vfs_mkdir
#include "kyrofs.h" // For kyrofs_dirent_t (temporary direct include)
#include "fs_disk.h" // For fs_format
#include <stdbool.h>
#include <string.h>

static int ide_ioctl(vfs_node_t *node, int request, void* argp); // Forward declaration

#pragma pack(push, 1)
typedef struct {
    uint8_t status;
    uint8_t chs_first_sector[3];
    uint8_t type;
    uint8_t chs_last_sector[3];
    uint32_t lba_first_sector;
    uint32_t sector_count;
} MBR_PartitionEntry;

typedef struct {
    uint8_t boot_code[446];
    MBR_PartitionEntry partitions[4];
    uint16_t boot_signature;
} MBR;
#pragma pack(pop)

// ATA Register Ports (Primary Bus)
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERROR       0x1F1
#define ATA_PRIMARY_SECCOUNT    0x1F2
#define ATA_PRIMARY_LBA_LO      0x1F3
#define ATA_PRIMARY_LBA_MID     0x1F4
#define ATA_PRIMARY_LBA_HI      0x1F5
#define ATA_PRIMARY_DRIVE_HEAD  0x1F6
#define ATA_PRIMARY_COMMAND     0x1F7
#define ATA_PRIMARY_STATUS      0x1F7

// Status Register Bits
#define ATA_SR_BSY  0x80 // Busy
#define ATA_SR_DRDY 0x40 // Drive Ready
#define ATA_SR_DF   0x20 // Drive Fault
#define ATA_SR_DRQ  0x08 // Data Request
#define ATA_SR_ERR  0x01 // Error

// Commands
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_CACHE_FLUSH 0xE7

static bool primary_master_present = false;

// Returns 0 on success, -1 on timeout
static int ide_wait_for_ready() {
    for(int i = 0; i < 100000; i++) {
        if (!(inb(ATA_PRIMARY_STATUS) & ATA_SR_BSY)) {
            return 0; // Success
        }
    }
    return -1; // Timeout
}

static int ide_probe(device_t* dev);
static int ide_attach(device_t* dev);

static driver_t ide_driver = {
    .name = "IDE Controller Driver",
    .probe = ide_probe,
    .attach = ide_attach,
    .next = NULL
};

static int ide_probe(device_t* dev) {
    if (dev->vendor_id == 0) return 0;
    uint8_t class_code = pci_get_class(dev->bus, dev->device, dev->func);
    uint8_t subclass_code = pci_get_subclass(dev->bus, dev->device, dev->func);
    if (class_code == 0x01 && subclass_code == 0x01) {
        klog(LOG_INFO, "IDE: Found an IDE controller.");
        return 1;
    }
    return 0;
}

static int ide_attach(device_t* dev) {
    klog(LOG_INFO, "IDE: Attaching to IDE controller.");
    dev->driver = &ide_driver;
    primary_master_present = false;

    // Select master drive
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0); 
    
    // Send IDENTIFY
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    // Check if drive exists
    if (inb(ATA_PRIMARY_STATUS) == 0) {
        klog(LOG_WARN, "IDE: Primary master drive does not exist.");
        return 0;
    }

    if (ide_wait_for_ready() != 0) {
        klog(LOG_WARN, "IDE: Timed out waiting for IDENTIFY.");
        return 0;
    }

    // Check for DRQ
    if (inb(ATA_PRIMARY_STATUS) & ATA_SR_DRQ) {
        klog(LOG_INFO, "IDE: Primary master drive found and ready.");
        primary_master_present = true;
            // Read and discard IDENTIFY data
            uint16_t identify_data[256];
            insw(ATA_PRIMARY_DATA, identify_data, 256);
        
            // Create a device node in KyroFS for this IDE drive
            // For simplicity, hardcode /dev/sda
            // In a real system, device numbering would be dynamic.
            vfs_node_t *dev_node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
            if (dev_node) {
                memset(dev_node, 0, sizeof(vfs_node_t));
                strncpy(dev_node->name, "sda", MAX_FILENAME_LEN);
                dev_node->flags = VFS_FILE; // Treat as a file for read/write blocks
                dev_node->read = (read_vfs_t)ide_read_sectors; // Cast to match signature
                dev_node->write = (write_vfs_t)ide_write_sectors; // Cast to match signature
                dev_node->ioctl = ide_ioctl; // Assign new ioctl handler
                dev_node->ptr = (void*)(uintptr_t)0; // Store drive number (0 for primary master)
        
                // For now, temporarily add the device node directly to vfs_root's children or similar.
                // A proper /dev filesystem or device registration mechanism is needed.
                // This is a placeholder to avoid compilation errors and will need proper VFS integration.
                klog(LOG_INFO, "IDE: /dev/sda device node created. (Needs proper VFS registration)");
            } else {
                klog(LOG_ERROR, "IDE: Failed to allocate vfs_node_t for /dev/sda.");
            }
            } else {
        klog(LOG_WARN, "IDE: Primary master drive not responding correctly to IDENTIFY.");
    }

    return 0;
}

// New IOCTL handler for IDE devices
static int ide_ioctl(vfs_node_t *node, int request, void* argp) {
    // For now, assume fixed primary master drive
    // The node->ptr could be used to store a drive ID if multiple IDE drives
    uint8_t drive = 0; 

    if (request == IDE_IOCTL_GET_DISK_INFO) {
        ide_disk_info_t *info = (ide_disk_info_t *)argp;
        if (info) {
            // Hardcode for now, should be obtained from IDENTIFY DEVICE
            info->total_sectors = 0x0FFFFFFF; // Example: large enough
            info->bytes_per_sector = 512;
            return 0;
        }
    } else if (request == IDE_IOCTL_READ_BLOCKS) {
        ide_read_blocks_t *rb = (ide_read_blocks_t *)argp;
        if (rb) {
            return ide_read_sectors(drive, rb->lba, rb->count, (uint16_t*)rb->buffer);
        }
    } else if (request == IDE_IOCTL_WRITE_BLOCKS) {
        ide_write_blocks_t *wb = (ide_write_blocks_t *)argp;
        if (wb) {
            return ide_write_sectors(drive, wb->lba, wb->count, (const uint16_t*)wb->buffer);
        }
    } else if (request == IDE_IOCTL_FORMAT) {
        ide_ioctl_format_t *fmt_req = (ide_ioctl_format_t *)argp;
        if (fmt_req) {
            // fs_format expects a disk_fd.
            // Since ide_ioctl is called from a vfs_node associated with a physical disk,
            // we can pass the node->ptr (which holds the drive number) as a pseudo-fd for fs_format,
            // or modify fs_format to take a vfs_node_t directly.
            // For now, let's assume fs_format can use the node->ptr as an identifier.
            // This is a simplification; a proper design might involve passing the vfs_node itself.
            return fs_format((int)(uintptr_t)node->ptr, fmt_req->partition_lba, fmt_req->partition_size);
        }
    }
    klog(LOG_WARN, "IDE: Unsupported IOCTL request %x", request);
    return -1;
}

void ide_test_read() {
    if (!primary_master_present) {
        klog(LOG_ERROR, "IDE Test: Aborting, primary master not found.");
        return;
    }

    uint16_t* buffer = (uint16_t*)kmalloc(512);
    if (!buffer) {
        klog(LOG_ERROR, "IDE Test: Failed to allocate buffer for MBR.");
        return;
    }

    klog(LOG_INFO, "IDE Test: Reading MBR from drive 0...");
    if (ide_read_sectors(0, 0, 1, buffer) != 0) {
        klog(LOG_ERROR, "IDE Test: Failed to read MBR.");
        kfree(buffer);
        return;
    }

    MBR* mbr = (MBR*)buffer;
    if (mbr->boot_signature != 0xAA55) {
        klog(LOG_WARN, "IDE Test: Invalid MBR signature.");
        kfree(buffer);
        return;
    }

    klog(LOG_INFO, "IDE Test: MBR signature is valid. Partitions:");
    char print_buf[128];
    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].type == 0) continue;
        ksprintf(print_buf, "  [%d] Type: 0x%x, Start LBA: %d, Size: %d sectors",
            i, (int)mbr->partitions[i].type, (int)mbr->partitions[i].lba_first_sector, (int)mbr->partitions[i].sector_count);
        klog(LOG_INFO, "%s", print_buf);
    }

    kfree(buffer);
}

int ide_read_sectors(uint8_t drive, uint32_t lba, uint8_t num_sectors, uint16_t* buffer) {
    if (drive != 0 || !primary_master_present) return -1;

    if (ide_wait_for_ready() != 0) return -1;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, num_sectors);
    outb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < num_sectors; i++) {
        if (ide_wait_for_ready() != 0) return -1;
        if (inb(ATA_PRIMARY_STATUS) & (ATA_SR_DF | ATA_SR_ERR)) {
            klog(LOG_ERROR, "IDE: Read error.");
            return -1;
        }
        insw(ATA_PRIMARY_DATA, buffer + (i * 256), 256);
    }
    return 0;
}

int ide_write_sectors(uint8_t drive, uint32_t lba, uint8_t num_sectors, const uint16_t* buffer) {
    if (drive != 0 || !primary_master_present) return -1;
    
    if (ide_wait_for_ready() != 0) return -1;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, num_sectors);
    outb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);

    for (int i = 0; i < num_sectors; i++) {
        if (ide_wait_for_ready() != 0) return -1;
        if (inb(ATA_PRIMARY_STATUS) & (ATA_SR_DF | ATA_SR_ERR)) {
            klog(LOG_ERROR, "IDE: Write error.");
            return -1;
        }
        outsw(ATA_PRIMARY_DATA, buffer + (i * 256), 256);
    }
    
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ide_wait_for_ready() != 0) return -1;
    
    return 0;
}

void ide_driver_init() {
    deviceman_register_driver(&ide_driver);
    klog(LOG_INFO, "IDE driver registered with Device Manager.");
}