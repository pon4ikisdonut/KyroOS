#ifndef LKM_H
#define LKM_H

#include <stdint.h>

// Module initialization function type
typedef int (*lkm_init_t)(void);
// Module exit function type
typedef void (*lkm_exit_t)(void);

// Macro to declare the init function
#define LKM_INIT(func) lkm_init_t __lkm_init_ptr = func;
// Macro to declare the exit function
#define LKM_EXIT(func) lkm_exit_t __lkm_exit_ptr = func;

// Function to load a kernel module from a memory buffer
int lkm_load(const uint8_t* module_data, size_t module_size);

#endif // LKM_H
