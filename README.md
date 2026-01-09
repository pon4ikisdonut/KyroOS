# KyroOS

**KyroOS** is a **full-featured operating system** for the **x86_64 architecture**, designed to deliver **maximum performance** and efficient hardware utilization. It features a **monolithic kernel written in C**, a higher-half memory layout, and a clean separation between kernel and userspace, implementing core OS concepts from the ground up.

## Philosophy

- **Hybrid UNIX-like**: Combines familiar UNIX concepts with custom design choices.  
- **Performance-first**: Designed to fully leverage modern hardware for maximum speed and efficiency.  
- **Security**: Implements kernel/userspace separation and basic integrity checks.  
- **Practicality**: Intended as a fully usable OS for users and developers, while still being open to experimentation and learning.

## Current Features (v26.01-alpha)

- **64-bit Kernel**: Runs in x86_64 long mode with higher-half memory layout.  
- **Bootloader**: GRUB-compliant, using the Multiboot2 specification.  
- **Memory Management**: Paging, physical memory manager (PMM), and kernel heap (`kmalloc`).  
- **Multitasking**: Preemptive round-robin scheduler with kernel threads and context switching.  
- **Drivers**:
  - Framebuffer graphics  
  - PS/2 Keyboard & Mouse  
  - PCI bus scanner  
  - Intel E1000 NIC  
  - AC'97 Audio  
- **Networking**: Minimal stack supporting ARP, IP, and ICMP (ping).  
- **Filesystem**: Virtual File System (VFS) with in-memory KyroFS implementation.  
- **Userspace**: ELF64 executable support with system calls.  
- **Graphics API**: Userspace library (`kyros_gfx`) for framebuffer access and input events, enabling simple graphical applications.

## Build and Run

Follow [BUILD.md](BUILD.md) for full instructions on setting up the cross-compiler and building the project.

### Quick Start
1. **Build**: `make all`  
2. **Run in QEMU**: `make run`

## Documentation

- **[Build Instructions](BUILD.md)**  
- **[System Architecture](doc/architecture.md)**  
- **[Syscall API](doc/syscalls.md)**  
- **[Package Format (.kpkg)](doc/kpkg_format.md)**  
- **[Kernel Module Format (.ko)](doc/module_format.md)**  
- **[Userspace Graphics Library API](doc/kyros_gfx_api.md)**
