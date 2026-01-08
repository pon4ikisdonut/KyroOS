#include "vfs.h"
#include "log.h"
#include <stddef.h> // for NULL

vfs_node_t* vfs_root = NULL;

void vfs_init() {
    // The root is set by mounting a filesystem, e.g., KyroFS
    klog(LOG_INFO, "VFS initialized.");
}

uint32_t vfs_read(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, char* name) {
    if (node && (node->flags & VFS_DIRECTORY) && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}
