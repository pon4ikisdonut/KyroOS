# KyroOS Architecture

This document provides a high-level overview of the KyroOS architecture as implemented in the initial prototype.

## 1. Core Design Philosophy

KyroOS is designed as a 64-bit, monolithic kernel with support for loadable modules. It follows a traditional user/kernel space separation (Ring 3 / Ring 0) and aims for high performance by providing low-level abstractions.

## 2. Boot Process

1.  **GRUB**: The system is booted by a GRUB-compliant bootloader using the Multiboot2 specification.
2.  **`boot.asm`**: The 32-bit entry point sets up a 64-bit environment.
    -   It configures page tables to map the kernel into the higher half of the virtual address space (`0xFFFFFFFF80000000`) and identity-maps the first 1GB of physical memory for accessing bootloader information and hardware.
    -   It enables PAE and Long Mode.
    -   It loads a temporary GDT and jumps to the 64-bit kernel entry point.
3.  **`kmain_x64`**: The main C entry point for the kernel. It initializes all subsystems and launches the first userspace process.

## 3. Kernel Subsystems

### Memory Management
-   **Physical Memory Manager (PMM)**: A bitmap-based allocator that tracks the usage of every 4KB physical page frame in memory. It gets the initial memory map from the Multiboot2 info provided by GRUB.
-   **Virtual Memory Manager (VMM)**: Manages the x86_64 page tables (PML4, PDPT, PD, PT). It provides an interface (`vmm_map_page`) to map virtual addresses to physical addresses.
-   **Kernel Heap**: A simple `kmalloc`/`kfree` implementation built on top of the PMM, allowing for dynamic allocation of memory within the kernel.

### Interrupts and Exceptions
-   **IDT**: The Interrupt Descriptor Table is populated with handlers for the first 32 CPU exceptions, hardware IRQs (PIC), and a syscall vector (`0x80`).
-   **ISRs**: Assembly stubs save the CPU state before calling a common C handler, which then dispatches to the appropriate driver or subsystem.
-   **Syscalls**: The `int 0x80` instruction is used as the userspace entry point into the kernel. A syscall table maps syscall numbers to kernel functions.

### Scheduling
-   **Threads**: The kernel supports a basic `thread_t` structure for multi-tasking.
-   **Scheduler**: A simple, preemptive, round-robin scheduler is implemented. The `schedule()` function is called by the timer interrupt (IRQ0).
-   **Context Switching**: A NASM assembly function (`thread_switch`) handles saving and restoring the register state between threads.

### Device Drivers
-   **Device Manager**: A generic framework allows drivers to be registered and devices to be discovered. The manager probes for device-driver matches and attaches the appropriate driver.
-   **PCI**: A PCI bus scanner enumerates devices by reading their configuration space. Discovered devices are registered with the device manager.
-   **Drivers Implemented**:
    -   PS/2 Keyboard & Mouse
    -   E1000 Network Interface Card
    -   AC'97 Audio Codec
    -   Framebuffer Graphics

## 4. Userspace

-   **ELF64 Loader**: The kernel contains a loader capable of parsing and mapping 64-bit ELF executables into virtual memory.
-   **Userspace Transition**: The kernel can transition from Ring 0 to Ring 3 using the `IRETQ` instruction, starting a userspace process with its own stack.
-   **Initial Process**: The kernel's final initialization step is to load and run the primary userspace application (e.g., `game.elf`), which is loaded as a module by GRUB.
