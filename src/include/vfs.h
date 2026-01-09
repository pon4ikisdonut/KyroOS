#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_FILENAME_LEN 256

struct vfs_node; // Forward declaration

// VFS file operations
typedef uint32_t (*read_vfs_t)(struct vfs_node* node, uint64_t offset, uint32_t size, uint8_t* buffer);
typedef uint32_t (*write_vfs_t)(struct vfs_node* node, uint64_t offset, uint32_t size, uint8_t* buffer);
typedef void (*open_vfs_t)(struct vfs_node* node, uint8_t read, uint8_t write);
typedef void (*close_vfs_t)(struct vfs_node* node);
typedef struct vfs_node* (*finddir_vfs_t)(struct vfs_node* node, char* name);
// ... other operations like mkdir, create, etc.

typedef struct vfs_node {
    char name[MAX_FILENAME_LEN];
    uint32_t flags;
    uint32_t length;
    uint32_t inode;
    void* ptr; // Used by mounted filesystems

    // Function pointers for file operations
    read_vfs_t read;
    write_vfs_t write;
    open_vfs_t open;
    close_vfs_t close;
    finddir_vfs_t finddir;
    
    // ... other function pointers
} vfs_node_t;

// Flags for VFS nodes
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_MOUNTPOINT  0x04

// Root of the VFS
extern vfs_node_t* vfs_root;

void vfs_init();
uint32_t vfs_read(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buffer);
vfs_node_t* vfs_finddir(vfs_node_t* node, char* name);

#endif // VFS_H
