#ifndef KYROFS_H
#define KYROFS_H

#include "vfs.h"

void kyrofs_init();
int kyrofs_add_file(char *path, void *data, uint32_t size);
vfs_node_t *get_kyrofs_root();

#endif // KYROFS_H
