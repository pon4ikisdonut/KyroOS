# KyroOS

KyroOS is a hobby operating system built from scratch for the x86_64 architecture. It is a monolithic kernel written in C, with a focus on implementing core OS concepts from the ground up.

## Philosophy

- **Hybrid UNIX-like**: Adopts familiar concepts while allowing for custom design choices.
- **Performance**: Aims for high performance with minimal overhead where possible.
- **Security**: Designed with basic security principles in mind, such as user/kernel space separation.
- **Education**: Primarily an educational project to explore and implement operating system internals.

## Current Features (as of v26.01-alpha)

- **64-bit Kernel**: Runs in x86_64 long mode with a higher-half memory layout.
- **Bootloader**: Boots via GRUB using the Multiboot2 specification.
- **Memory Management**: Paging, a physical memory manager (PMM), and a kernel heap (`kmalloc`).
- **Multitasking**: A preemptive round-robin scheduler for kernel threads.
- **Drivers**:
  - Framebuffer graphics driver.
  - PS/2 Keyboard and Mouse drivers.
  - PCI bus scanner.
  - E1000 NIC driver.
  - AC'97 Audio driver.
- **Networking**: A minimal stack supporting ARP, IP, and ICMP (ping).
- **Filesystem**: A virtual file system (VFS) with an initial in-memory filesystem (KyroFS).
- **Userspace**: Ability to load and run ELF64 executables, with a syscall interface for kernel communication.
- **Simple Graphics API**: A userspace library (`kyros_gfx`) provides access to the framebuffer and input events, allowing for simple graphical applications.

## Building and Running

For full instructions on how to set up a cross-compiler and build the project, see [BUILD.md](BUILD.md).

### Quick Start
1.  **Build**: `make all`
2.  **Run in QEMU**: `make run`

## Documentation

- **[Build Instructions](BUILD.md)**
- **[System Architecture](doc/architecture.md)**
- **[Syscall API](doc/syscalls.md)**
- **[Package Format](doc/kpkg_format.md)**
- **[Module Format](doc/module_format.md)**
- **[Graphics Library API](doc/kyros_gfx_api.md)**
