# KyroOS - A Modern x86-64 Operating System

KyroOS is an experimental 64-bit operating system for the x86-64 architecture, developed as a learning project. It features a monolithic kernel designed for simplicity, readability, and extensibility.

## Key Features

*   **x86-64 Architecture:** Leverages modern 64-bit computing capabilities.
*   **Monolithic Kernel:** Simple, readable, and educational kernel design.
*   **Memory Management:** Physical Memory Manager (PMM) and Virtual Memory Manager (VMM) with demand paging.
*   **Multitasking:** Preemptive, round-robin scheduler for efficient task switching.
*   **Basic Drivers:** Support for keyboard, mouse, framebuffer, IDE hard disks, E1000 network cards, and AC97 audio.
*   **Virtual File System (VFS):** Unified interface for file operations, with KyroFS (in-memory and disk-based) as the primary filesystem.
*   **Userspace Environment:** A basic shell (`kyroshell`) and a set of core utilities (`cat`, `ls`, `mkdir`, etc.).
*   **Networking Stack:** Support for ARP, IP, UDP, and a stubbed TCP implementation.
*   **Package Manager (`kpm`):** Simple command-line tool for managing userspace software.
*   **Installer:** An automated installer to prepare disk partitions.

## System Requirements

*   **CPU:** x86-64 compatible processor.
*   **RAM:** Minimum 64 MB.
*   **Storage:** Minimum 100 MB for installation.
*   **Boot:** BIOS or UEFI firmware.

## Quick Build Overview

KyroOS is built using a standard GNU toolchain (GCC, Make, NASM, xorriso). It can be built on a native Linux environment or on Windows Subsystem for Linux (WSL).

1.  **Install Toolchain:** On a Debian-based system (like Ubuntu or WSL):
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential nasm xorriso gcc-x86-64-linux-gnu
    ```
2.  **Build Project:** Navigate to the project root and run:
    ```bash
    make all
    ```
    This will compile the kernel and userspace programs, and create `kyroos.iso`.
## Documentation

The full documentation for KyroOS can be found in the `docs_en/` or `docs_ru/` directory.

## Warning

KyroOS is under heavy development and is considered an **alpha** quality operating system. It is not stable, may contain significant bugs, and is not suitable for production use.
