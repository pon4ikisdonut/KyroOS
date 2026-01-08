# KyroOS Syscall API

This document lists the available system calls for userspace programs.

- **Syscall Vector**: `int 0x80`
- **Register Convention**:
    - `rax`: Syscall number
    - `rdi`, `rsi`, `rdx`: Arguments 1, 2, 3
    - `rax`: Return value

---

### `SYS_EXIT` (0)
Terminates the current process.

- **Arguments**: None.
- **Returns**: Does not return.

---

### `SYS_WRITE` (1)
Writes to a file descriptor. Currently, only `fd=1` (stdout) is supported, which prints to the kernel log.

- **`rdi` (arg1)**: `int fd` - The file descriptor.
- **`rsi` (arg2)**: `const char*` - Pointer to the user buffer.
- **`rdx` (arg3)**: `size_t` - Number of bytes to write.
- **Returns**: Number of bytes written, or -1 on error.

---

### `SYS_OPEN` (2) - STUB
*This syscall is not yet implemented.*
Opens a file.

- **`rdi` (arg1)**: `const char* path`
- **`rsi` (arg2)**: `int flags`
- **Returns**: A file descriptor, or -1 on error.

---

### `SYS_READ` (3) - STUB
*This syscall is not yet implemented.*
Reads from a file descriptor.

- **`rdi` (arg1)**: `int fd`
- **`rsi` (arg2)**: `void* buf`
- **`rdx` (arg3)**: `size_t count`
- **Returns**: Number of bytes read, or -1 on error.

---

### `SYS_STAT` (4) - STUB
*This syscall is not yet implemented.*
Gets file status.

- **`rdi` (arg1)**: `const char* path`
- **`rsi` (arg2)**: `struct stat* buf`
- **Returns**: 0 on success, -1 on error.

---

### `SYS_GFX_GET_FB_INFO` (5)
Gets the current framebuffer information.

- **`rdi` (arg1)**: `fb_info_t*` - Pointer to a user-allocated struct to be filled by the kernel.
- **Returns**: 0 on success, -1 on error.

---

### `SYS_INPUT_POLL_EVENT` (6)
Pops the next event from the kernel's input event queue.

- **`rdi` (arg1)**: `event_t*` - Pointer to a user-allocated struct to be filled by the kernel.
- **Returns**: 1 if an event was popped, 0 if the queue was empty.
