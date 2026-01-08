#include "kyrofs.h"
#include "vfs.h"
#include "heap.h"
#include "log.h"
#include <stddef.h> // for NULL

// This is a simple in-memory implementation of KyroFS for now.

// Simple string copy
static void kstrcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Simple string compare
static int kstrcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// KyroFS directory entry structure
typedef struct kyrofs_dirent {
    vfs_node_t node;
    struct kyrofs_dirent* next;
} kyrofs_dirent_t;

// KyroFS file content structure
typedef struct {
    uint8_t* content;
    uint32_t size;
} kyrofs_file_content_t;

// KyroFS function implementations
static uint32_t kyrofs_read(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buffer) {
    kyrofs_file_content_t* file_content = (kyrofs_file_content_t*)node->ptr;
    if (offset + size > file_content->size) {
        size = file_content->size - offset;
    }
    if (offset > file_content->size) {
        return 0;
    }

    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = file_content->content[offset + i];
    }
    return size;
}

static vfs_node_t* kyrofs_finddir(vfs_node_t* node, char* name) {
    kyrofs_dirent_t* dir_list = (kyrofs_dirent_t*)node->ptr;
    kyrofs_dirent_t* current = dir_list;
    while (current) {
        if (kstrcmp(current->node.name, name) == 0) {
            return &current->node;
        }
        current = current->next;
    }
    return NULL;
}

static kyrofs_dirent_t* root_dir = NULL;

void kyrofs_init() {
    // Create root directory
    root_dir = (kyrofs_dirent_t*)kmalloc(sizeof(kyrofs_dirent_t));
    kstrcpy(root_dir->node.name, "/");
    root_dir->node.flags = VFS_DIRECTORY;
    root_dir->node.finddir = kyrofs_finddir;
    root_dir->next = NULL;
    
    // Create a hello.txt file in the root directory
    kyrofs_dirent_t* hello_file_dirent = (kyrofs_dirent_t*)kmalloc(sizeof(kyrofs_dirent_t));
    kstrcpy(hello_file_dirent->node.name, "hello.txt");
    hello_file_dirent->node.flags = VFS_FILE;
    hello_file_dirent->node.read = kyrofs_read;
    hello_file_dirent->next = NULL;
    
    kyrofs_file_content_t* hello_content = (kyrofs_file_content_t*)kmalloc(sizeof(kyrofs_file_content_t));
    char* content_str = "Hello from KyroFS!";
    hello_content->size = 18;
    hello_content->content = (uint8_t*)kmalloc(hello_content->size);
    for(int i = 0; i < 18; i++) hello_content->content[i] = content_str[i];
    
    hello_file_dirent->node.ptr = hello_content;
    hello_file_dirent->node.length = hello_content->size;

    // Link hello.txt into the root directory's list
    root_dir->node.ptr = hello_file_dirent;

    // Set the VFS root
    vfs_root = &root_dir->node;

    klog(LOG_INFO, "In-memory KyroFS initialized and mounted as root.");
}

vfs_node_t* get_kyrofs_root() {
    return &root_dir->node;
}


// sorry my cat laying on keyboard again