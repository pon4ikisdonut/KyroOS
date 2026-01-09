#include "kstring.h"
#include <stdint.h> // For uint8_t
#include <stdarg.h> // For va_list

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

// Helper function to convert integer to string and return length
static int int_to_str(int n, char s[]) {
    int i = 0;
    int is_negative = 0;
    if (n < 0) {
        is_negative = 1;
        n = -n;
    }
    
    // Handle 0 explicitly, otherwise it won't print "0"
    if (n == 0) {
        s[i++] = '0';
        s[i] = '\0';
        return 1;
    }

    while (n != 0) {
        s[i++] = (n % 10) + '0';
        n /= 10;
    }

    if (is_negative) {
        s[i++] = '-';
    }

    s[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    char temp;
    while (start < end) {
        temp = s[start];
        s[start] = s[end];
        s[end] = temp;
        start++;
        end--;
    }
    return i; // Return length of the string
}

int ksprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char* buf_ptr = buffer;
    char num_buf[12]; // Enough for a 32-bit integer, plus null terminator and sign

    while (*format) {
        if (*format == '%') {
            format++;
            int padding = 0;
            char pad_char = ' ';

            // Check for padding like %02d or %04d
            if (*format == '0') {
                format++;
                if (*format >= '0' && *format <= '9') {
                    padding = *format - '0';
                    format++;
                    if (*format >= '0' && *format <= '9') { // For %04d
                        padding = padding * 10 + (*format - '0');
                        format++;
                    }
                    pad_char = '0';
                }
            }

            switch (*format) {
                case 's': {
                    char* s = va_arg(args, char*);
                    while (*s) {
                        *buf_ptr++ = *s++;
                    }
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    int len = int_to_str(d, num_buf);
                    
                    // Apply padding
                    for (int i = 0; i < padding - len; i++) {
                        *buf_ptr++ = pad_char;
                    }

                    for (int i = 0; i < len; i++) {
                        *buf_ptr++ = num_buf[i];
                    }
                    break;
                }
                default:
                    if (pad_char == '0' && padding > 0) { // If it was a %0X but not %0Xd
                        *buf_ptr++ = '%';
                        *buf_ptr++ = '0';
                        if (padding >= 10) *buf_ptr++ = (padding / 10) + '0';
                        *buf_ptr++ = (padding % 10) + '0';
                    } else { // Normal % or unknown specifier
                        *buf_ptr++ = '%';
                    }
                    *buf_ptr++ = *format;
                    break;
            }
        } else {
            *buf_ptr++ = *format;
        }
        format++;
    }
    *buf_ptr = '\0';
    va_end(args);
    return buf_ptr - buffer; // Return number of characters written
}