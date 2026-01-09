#ifndef KSTRING_H
#define KSTRING_H

#include <stddef.h>
#include <stdarg.h> // For va_list

void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);

// GCC may optimize to these, so we provide them.
void* __memcpy_chk(void* dest, const void* src, size_t n, size_t dest_len);

// Basic sprintf implementation
int ksprintf(char* buffer, const char* format, ...);

#endif // KSTRING_H
