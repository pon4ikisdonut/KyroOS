#include "kstring.h"
#include <stdint.h> // For uint8_t

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* pdest = (uint8_t*)dest;
    const uint8_t* psrc = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

// Simple implementation of __memcpy_chk that just calls our memcpy
void* __memcpy_chk(void* dest, const void* src, size_t n, size_t dest_len) {
    // A real implementation would check if n > dest_len and abort.
    (void)dest_len; // Suppress unused parameter warning
    return memcpy(dest, src, n);
}