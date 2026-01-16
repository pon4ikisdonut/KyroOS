#ifndef KYROLIB_H
#define KYROLIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h> // For va_list, va_start, va_arg, va_end
#include "vfs.h" // For O_* flags
#include "event.h" // For event_t
#include "syscall.h" // For SYS_* macros
#include "socket.h" // For sockaddr_in

// Standard open flags (simplified)
#define O_RDONLY    0x0001 // Open for reading only
#define O_WRONLY    0x0002 // Open for writing only
#define O_RDWR      0x0003 // Open for reading and writing
#define O_CREAT     0x0040 // Create file if it does not exist
#define O_TRUNC     0x0200 // Truncate size to 0
#define O_APPEND    0x0400 // Append mode

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_OPEN 2      // (const char *path, int flags)
#define SYS_CLOSE 3     // (int fd)
#define SYS_READ 4      // (int fd, void *buf, size_t size)
#define SYS_STAT 5      // (const char *path, struct stat *stat_buf)
#define SYS_MKDIR 6     // (const char *path)
#define SYS_READDIR 7   // (int fd, int index, struct dirent *dir_entry)
#define SYS_UNLINK 8    // (const char *path)
#define SYS_RMDIR 9     // (const char *path)
#define SYS_SOCKET 10   // (int domain, int type, int protocol)
#define SYS_CONNECT 11  // (int sockfd, const struct sockaddr_in *addr, size_t addrlen)
#define SYS_SEND 12     // (int sockfd, const void *buf, size_t len, int flags)
#define SYS_RECV 13     // (int sockfd, void *buf, size_t len, int flags)
#define SYS_BIND 14     // (int sockfd, const struct sockaddr_in *addr, size_t addrlen)
#define SYS_LISTEN 15   // (int sockfd, int backlog)
#define SYS_ACCEPT 16   // (int sockfd, struct sockaddr_in *addr, size_t *addrlen)
#define SYS_GET_TICKS 17
#define SYS_SHA256 18 // (const uint8_t* data, size_t len, uint8_t* hash_out)
#define SYS_MALLOC 19 // (size_t size)
#define SYS_FREE 20   // (void *ptr)
#define SYS_IOCTL 21 // (int fd, int request, void* argp)
#define SYS_MOUNT 22 // (const char* mount_point_path, const char* device_node_path, const char* fs_type_name)
#define SYS_UNMOUNT 23 // (const char* mount_point_path)
#define SYS_EXEC 24
#define SYS_GFX_GET_FB_INFO 25
#define SYS_INPUT_POLL_EVENT 26

static inline uint64_t syscall(uint64_t num, uint64_t a1, uint64_t a2,
                               uint64_t a3) {
  uint64_t ret;
  __asm__ __volatile__("movq %1, %%rax\n\t"
                       "movq %2, %%rdi\n\t"
                       "movq %3, %%rsi\n\t"
                       "movq %4, %%rdx\n\t"
                       "int $0x80\n\t"
                       "movq %%rax, %0"
                       : "=r"(ret)
                       : "r"(num), "r"(a1), "r"(a2), "r"(a3)
                       : "rax", "rdi", "rsi", "rdx", "memory");
  return ret;
}

static inline void exit() { syscall(SYS_EXIT, 0, 0, 0); }
static inline void print(const char *s) {
  syscall(SYS_WRITE, 1, (uint64_t)s, 0);
}
static inline int open(const char *path, int flags) {
  return (int)syscall(SYS_OPEN, (uint64_t)path, (uint64_t)flags, 0);
}
static inline int close(int fd) {
  return (int)syscall(SYS_CLOSE, (uint64_t)fd, 0, 0);
}
static inline int read(int fd, void *buf, size_t size) {
  return (int)syscall(SYS_READ, fd, (uint64_t)buf, size);
}

static inline int write(int fd, const void *buf, size_t size) {
  return (int)syscall(SYS_WRITE, fd, (uint64_t)buf, size);
}
static inline int mkdir(const char *path) {
  return (int)syscall(SYS_MKDIR, (uint64_t)path, 0, 0);
}
static inline int stat(const char *path, struct stat *stat_buf) { // New stat wrapper
  return (int)syscall(SYS_STAT, (uint64_t)path, (uint64_t)stat_buf, 0);
}
static inline int unlink(const char *path) {
  return (int)syscall(SYS_UNLINK, (uint64_t)path, 0, 0);
}
static inline int rmdir(const char *path) { // New wrapper for SYS_RMDIR
  return (int)syscall(SYS_RMDIR, (uint64_t)path, 0, 0);
}
static inline int readdir(int fd, int index, struct dirent *dir_entry) {
  return (int)syscall(SYS_READDIR, (uint64_t)fd, (uint64_t)index, (uint64_t)dir_entry);
}
static inline int create(const char *path) {
    return open(path, O_CREAT | O_TRUNC | O_WRONLY); // Using new open
}

// New socket wrappers
static inline int socket(int domain, int type, int protocol) {
  return (int)syscall(SYS_SOCKET, (uint64_t)domain, (uint64_t)type, (uint64_t)protocol);
}
static inline int bind(int sockfd, const struct sockaddr_in *addr, size_t addrlen) {
  return (int)syscall(SYS_BIND, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)addrlen);
}
static inline int listen(int sockfd, int backlog) {
  return (int)syscall(SYS_LISTEN, (uint64_t)sockfd, (uint64_t)backlog, 0);
}
static inline int accept(int sockfd, struct sockaddr_in *addr, size_t *addrlen) {
  return (int)syscall(SYS_ACCEPT, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)addrlen);
}
static inline int connect(int sockfd, const struct sockaddr_in *addr, size_t addrlen) {
  return (int)syscall(SYS_CONNECT, (uint64_t)sockfd, (uint64_t)addr, (uint64_t)addrlen);
}
static inline int send(int sockfd, const void *buf, size_t len, int /*flags*/) {
    return syscall(SYS_SEND, sockfd, (uint64_t)buf, len);
}
static inline int recv(int sockfd, void *buf, size_t len, int /*flags*/) {
    return syscall(SYS_RECV, sockfd, (uint64_t)buf, len);
}

static inline int sha256(const uint8_t* data, size_t len, uint8_t* hash_out) {
  return (int)syscall(SYS_SHA256, (uint64_t)data, (uint64_t)len, (uint64_t)hash_out);
}

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    void* buffer;
} user_fb_info_t;

static inline int gfx_get_fb_info(user_fb_info_t *info_ptr) {
    return (int)syscall(SYS_GFX_GET_FB_INFO, (uint64_t)info_ptr, 0, 0);
}



static inline int ioctl(int fd, int request, void* argp) {
    return (int)syscall(SYS_IOCTL, (uint64_t)fd, (uint64_t)request, (uint64_t)argp);
}

static inline int mount(const char* mount_point_path, const char* device_node_path, const char* fs_type_name) {
    return (int)syscall(SYS_MOUNT, (uint64_t)mount_point_path, (uint64_t)device_node_path, (uint64_t)fs_type_name);
}

static inline int unmount(const char* mount_point_path) {
    return (int)syscall(SYS_UNMOUNT, (uint64_t)mount_point_path, 0, 0);
}

static inline int input_poll_event(event_t *event) {
    return (int)syscall(SYS_INPUT_POLL_EVENT, (uint64_t)event, 0, 0);
}

static inline int atoi(const char *s) {
    int res = 0;
    int sign = 1;
    while (*s == ' ') s++; // Skip leading whitespace
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

static inline uint64_t get_ticks() { return syscall(SYS_GET_TICKS, 0, 0, 0); }

// Minimal string/memory functions
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int memcmp(const void *s1, const void *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strncpy(char *dest, const char *src, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

// Basic sprintf implementation for userspace (adapted from kernel/string.c)
static inline int vksprintf(char *buffer, const char *format, va_list args) {
    char *buf_ptr = buffer;
    int written = 0;

    // A minimal implementation, similar to kernel's
    // Handles %s, %d, %x, %c, %%
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char *s = va_arg(args, char *);
                while (*s) {
                    *buf_ptr++ = *s++;
                    written++;
                }
            } else if (*format == 'd') {
                int d = va_arg(args, int);
                char num_buf[12]; // Max for int + sign
                int i = 0;
                if (d == 0) {
                    num_buf[i++] = '0';
                } else {
                    int is_negative = 0;
                    if (d < 0) {
                        is_negative = 1;
                        d = -d;
                    }
                    while (d > 0) {
                        num_buf[i++] = (d % 10) + '0';
                        d /= 10;
                    }
                    if (is_negative) {
                        num_buf[i++] = '-';
                    }
                }
                while (i > 0) {
                    *buf_ptr++ = num_buf[--i];
                    written++;
                }
            } else if (*format == 'x') { // Hexadecimal
                unsigned int x = va_arg(args, unsigned int);
                char hex_buf[9]; // Max for uint32_t (8 chars) + null
                int i = 0;
                if (x == 0) {
                    hex_buf[i++] = '0';
                } else {
                    while (x > 0) {
                        int digit = x % 16;
                        if (digit < 10) {
                            hex_buf[i++] = digit + '0';
                        } else {
                            hex_buf[i++] = digit - 10 + 'a';
                        }
                        x /= 16;
                    }
                }
                while (i > 0) {
                    *buf_ptr++ = hex_buf[--i];
                    written++;
                }
            } else if (*format == 'c') {
                char c = va_arg(args, int); // Chars are promoted to int
                *buf_ptr++ = c;
                written++;
            } else if (*format == '%') {
                *buf_ptr++ = '%';
                written++;
            }
        } else {
            *buf_ptr++ = *format;
            written++;
        }
        format++;
    }
    *buf_ptr = '\0';
    return written;
}

static inline int ksprintf(char *buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vksprintf(buffer, format, args);
    va_end(args);
    return ret;
}

static inline void *malloc(size_t size) {
    return (void *)syscall(SYS_MALLOC, (uint64_t)size, 0, 0);
}

static inline void free(void *ptr) {
    syscall(SYS_FREE, (uint64_t)ptr, 0, 0);
}

#endif