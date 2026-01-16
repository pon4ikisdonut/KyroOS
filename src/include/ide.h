#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#include "vfs.h" // For vfs_node_t and ioctl_vfs_t

// IOCTL Requests for IDE driver
#define IDE_IOCTL_GET_DISK_INFO 0x01 // Get disk geometry (total sectors, sector size)
typedef struct {
    uint64_t total_sectors;
    uint32_t bytes_per_sector;
} ide_disk_info_t;

#define IDE_IOCTL_READ_BLOCKS   0x02 // Read blocks (LBA, count, buffer)
typedef struct {
    uint32_t lba;
    uint32_t count;
    void* buffer;
} ide_read_blocks_t;

#define IDE_IOCTL_WRITE_BLOCKS  0x03 // Write blocks (LBA, count, buffer)
#define IDE_IOCTL_FORMAT        0x04 // Format partition (disk_fd, partition_lba, partition_size)
typedef struct {
    uint32_t lba;
    uint32_t count;
    const void* buffer;
} ide_write_blocks_t;

typedef struct {
    uint32_t partition_lba;
    uint32_t partition_size;
} ide_ioctl_format_t;

void ide_driver_init();
int ide_read_sectors(uint8_t drive, uint32_t lba, uint8_t num_sectors, uint16_t* buffer);
int ide_write_sectors(uint8_t drive, uint32_t lba, uint8_t num_sectors, const uint16_t* buffer);
void ide_test_read();

#endif // IDE_H
