# KyroOS Build System

# Default target
.PHONY: all
all: kyros.iso

# Toolchain
TARGET = x86_64-elf
CC = $(TARGET)-gcc
AS = nasm # Use NASM for assembly files
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy

# Compilation flags
CFLAGS = -Wall -Wextra -std=c11 -ffreestanding -O2 -Isrc/include -mcmodel=large -mno-red-zone -m64
ASFLAGS = -felf64
NASMFLAGS = -f elf64

# Project structure
BUILD_DIR = build
SRC_DIR = src
ISO_DIR = build/isodir

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/kernel/**/*.c)
ASM_SOURCES = $(wildcard $(SRC_DIR)/boot/**/*.asm)

# Object files
C_OBJS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ASM_OBJS = $(ASM_SOURCES:$(SRC_DIR)/%.asm=$(BUILD_DIR)/%.o)
OBJS = $(C_OBJS) $(ASM_OBJS)

KERNEL = build/kyros.elf

# Build target: KyroOS ISO
kyros.iso: $(KERNEL) grub/grub.cfg
	@echo "Creating KyroOS bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL) $(ISO_DIR)/boot/kyros.elf
	@cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o kyros.iso $(ISO_DIR)

# Link the kernel
$(KERNEL): $(OBJS) linker.ld
	@echo "Linking kernel..."
	@$(LD) -T linker.ld -o $@ $(OBJS) -m elf_x86_64

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "Compiling C: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile NASM Assembly source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(@D)
	@echo "Compiling ASM: $<"
	@$(AS) $(NASMFLAGS) $< -o $@

# QEMU runner
.PHONY: run
run: kyros.iso
	@qemu-system-x86_64 -cdrom kyros.iso

# Clean up build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build kyros.iso
