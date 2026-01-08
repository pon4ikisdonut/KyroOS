# KyroOS Build Instructions

This document provides instructions on how to set up the build environment, compile the KyroOS kernel, and run it in an emulator.

## 1. Prerequisites

You must have the following tools installed on your system (preferably a Linux-like environment):

- `make`: The GNU Make utility to automate the build process.
- `nasm`: The Netwide Assembler for compiling assembly files.
- `qemu-system-x86_64`: An emulator to run the x86_64 version of KyroOS.
- `grub-mkrescue`: A tool to create a bootable GRUB ISO image. This often requires the `xorriso` package.
- An `x86_64-elf` cross-compiler toolchain (GCC, Binutils).

## 2. Cross-Compiler Toolchain

A cross-compiler is required to build the kernel for the `x86_64-elf` target. If you do not have one, you can build it from source.

For instructions, refer to the OSDev Wiki guide: [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler).

Ensure the `bin` directory of your new toolchain is in your system's `PATH`. For example:
```sh
export PATH="/path/to/cross/compiler/bin:$PATH"
```

## 3. Project Structure

The project follows this basic structure:
```
kyros/
├── build/             # Build artifacts (will be created)
├── grub/
│   └── grub.cfg       # GRUB bootloader configuration
├── src/
│   ├── boot/          # Assembly boot code (boot.asm, isr_stubs.asm)
│   ├── include/       # Kernel header files
│   └── kernel/        # Kernel C source code
├── .gitignore
├── BUILD.md           # This file
├── do.txt             # Project state tracker
├── Makefile           # Main build file
└── linker.ld          # Kernel linker script
```

## 4. Building KyroOS

To compile the entire kernel and create a bootable ISO image, run the main `make` command from the root of the project directory:

```sh
make all
```
Alternatively, you can run:
```sh
make kyros.iso
```
This will:
1. Compile all assembly (`.asm`) and C (`.c`) source files into object files and place them in the `build/` directory.
2. Link the object files into the final kernel ELF executable (`build/kyros.elf`).
3. Create a bootable ISO image named `kyros.iso` in the project root, containing the GRUB bootloader and the kernel.

## 5. Running KyroOS in QEMU

After a successful build, you can run KyroOS in the QEMU emulator with the following command:

```sh
make run
```

This will launch `qemu-system-x86_64` and boot from the `kyros.iso` file.

## 6. Cleaning the Build

To remove all build artifacts (the `build/` directory and `kyros.iso`), run:

```sh
make clean
```
