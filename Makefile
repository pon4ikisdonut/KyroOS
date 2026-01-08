# KyroOS Build System

# Default target
.PHONY: all
all: kyros.iso

# Toolchain
TARGET = x86_64-elf
CC = $(TARGET)-gcc
AR = $(TARGET)-ar
AS = nasm
LD = $(TARGET)-ld

# Kernel Compilation flags
K_CFLAGS = -Wall -Wextra -std=c11 -ffreestanding -O2 -Isrc/include -mcmodel=large -mno-red-zone -m64
NASMFLAGS = -f elf64

# Userspace Compilation flags
U_CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -Iuserspace/lib -ffreestanding -nostdlib

# Project structure
BUILD_DIR = build
SRC_DIR = src
ISO_DIR = build/isodir
USER_DIR = userspace
MODULES_DIR = modules

# Kernel files
K_C_SOURCES = $(wildcard $(SRC_DIR)/kernel/**/*.c)
K_ASM_SOURCES = $(wildcard $(SRC_DIR)/boot/**/*.asm)
K_C_OBJS = $(K_C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/kernel/%.o)
K_ASM_OBJS = $(K_ASM_SOURCES:$(SRC_DIR)/%.asm=$(BUILD_DIR)/kernel/%.o)
K_OBJS = $(K_C_OBJS) $(K_ASM_OBJS)
KERNEL = build/kyros.elf

# Userspace files
INIT_ELF = $(BUILD_DIR)/userspace/init.elf
INIT_OBJ = $(BUILD_DIR)/userspace/init.o
GAME_ELF = $(BUILD_DIR)/userspace/game.elf
GAME_OBJ = $(BUILD_DIR)/userspace/game.o
KYROS_GFX_LIB = $(BUILD_DIR)/userspace/libkyros_gfx.a
KYROS_GFX_OBJ = $(BUILD_DIR)/userspace/kyros_gfx.o

# Module files
HELLO_LKM = $(MODULES_DIR)/hello_lkm/hello.ko

# Build target: KyroOS ISO
kyros.iso: $(KERNEL) $(GAME_ELF) $(HELLO_LKM) grub/grub.cfg
	@echo "Creating KyroOS bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@mkdir -p $(ISO_DIR)/boot/modules
	@cp $(KERNEL) $(ISO_DIR)/boot/kyros.elf
	@cp $(GAME_ELF) $(ISO_DIR)/boot/game.elf
	@cp $(HELLO_LKM) $(ISO_DIR)/boot/modules/hello.ko
	@cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o kyros.iso $(ISO_DIR)

# --- Kernel Build ---
$(KERNEL): $(K_OBJS) linker.ld
	@$(LD) -T linker.ld -o $@ $(K_OBJS) -m elf_x86_64
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(K_CFLAGS) -c $< -o $@
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(@D)
	@$(AS) $(NASMFLAGS) $< -o $@

# --- Userspace Build ---
# Game
$(GAME_ELF): $(GAME_OBJ) $(KYROS_GFX_LIB)
	@$(LD) -T $(USER_DIR)/game/link.ld -o $@ $< -L$(BUILD_DIR)/userspace -lkyros_gfx
$(GAME_OBJ): $(USER_DIR)/game/game.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# GFX Library
$(KYROS_GFX_LIB): $(KYROS_GFX_OBJ)
	@$(AR) rcs $@ $<
$(KYROS_GFX_OBJ): $(USER_DIR)/lib/kyros_gfx/kyros_gfx.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# --- Module Build ---
.PHONY: modules
modules: $(HELLO_LKM)
$(HELLO_LKM):
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm

# --- Misc ---
.PHONY: run
run: kyros.iso
	@qemu-system-x86_64 -cdrom kyros.iso

.PHONY: clean
clean:
	@rm -rf build kyros.iso
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm clean
