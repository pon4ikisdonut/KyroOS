#include "kstring.h"
#include <stdbool.h>
#include <stdarg.h> // For va_list
#include <stdint.h> // For uint8_t

void *memcpy(void *dest, const void *src, size_t n) {
    __asm__ __volatile__ (
        "cld;"
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(n)
        :
        : "memory"
    );
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d == s) {
        return d;
    }
    if (d < s) {
        // Copy forwards
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        // Copy backwards
        for (size_t i = n; i != 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = (uint8_t *)s;
  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }
  return s;
}

// Simple implementation of __memcpy_chk that just calls our memcpy
void *__memcpy_chk(void *dest, const void *src, size_t n, size_t dest_len) {
  // A real implementation would check if n > dest_len and abort.
  (void)dest_len; // Suppress unused parameter warning
  return memcpy(dest, src, n);
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
    n--;
  }
  if (n == 0)
    return 0;
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
  for (; i < n; i++)
    dest[i] = '\0';
  return dest;
}

char *strchr(const char *s, int c) {
  while (*s != (char)c) {
    if (!*s++) {
      return NULL;
    }
  }
  return (char *)s;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    do {
        if (*s == (char)c) {
            last = s;
        }
    } while (*s++);
    return (char *)last;
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

int ksprintf(char *buffer, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vksprintf(buffer, format, args);
  va_end(args);
  return ret;
}

int vksprintf(char *buffer, const char *format, va_list args) {
  char *buf_ptr = buffer;
  char
      num_buf[12]; // Enough for a 32-bit integer, plus null terminator and sign

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

      // Check for 'l' modifier for 64-bit hex
      bool is_long = false;
      if (*format == 'l') {
          is_long = true;
          format++;
      }

      switch (*format) {
      case 's': {
        char *s = va_arg(args, char *);
        if (!s)
          s = "(null)";
        while (*s) {
          *buf_ptr++ = *s++;
        }
        break;
      }
      case 'p':
      case 'x': {
        uint64_t val;
        if (*format == 'p') {
          val = (uint64_t)va_arg(args, void *);
          *buf_ptr++ = '0';
          *buf_ptr++ = 'x';
        } else {
          if (is_long) {
            val = va_arg(args, uint64_t);
          } else {
            val = va_arg(args, uint32_t); // Standard %x is 32-bit
          }
        }

        char hex_chars[] = "0123456789abcdef";
        char hex_buf[16];
        int i = 0;
        if (val == 0) {
          hex_buf[i++] = '0';
        } else {
          while (val > 0) {
            hex_buf[i++] = hex_chars[val % 16];
            val /= 16;
          }
        }

        // Simple padding for hex (standard)
        int h_padding = padding > i ? padding : i;
        for (int j = 0; j < h_padding - i; j++) {
          *buf_ptr++ = pad_char;
        }
        while (i > 0) {
          *buf_ptr++ = hex_buf[--i];
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
          if (padding >= 10)
            *buf_ptr++ = (padding / 10) + '0';
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
  return buf_ptr - buffer; // Return number of characters written
}