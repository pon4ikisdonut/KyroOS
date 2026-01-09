# KyroOS Build System (Optimized for WSL + Windows QEMU)

# --- Toolchain Setup ---
# В Ubuntu/WSL пакет называется x86_64-linux-gnu, он отлично подходит
# при использовании флагов -ffreestanding и -nostdlib
TARGET = x86_64-linux-gnu
CC = $(TARGET)-gcc
AR = $(TARGET)-ar
AS = nasm
LD = $(TARGET)-ld

# --- Compilation Flags ---
# Добавил -nostdlib и -fno-stack-protector (важно для ядра)
K_CFLAGS = -Wall -Wextra -std=c11 -ffreestanding -O2 -Isrc/include \
           -mcmodel=large -mno-red-zone -m64 -nostdlib -fno-stack-protector
NASMFLAGS = -f elf64

# Userspace flags
U_CFLAGS = -Wall -Wextra -std=gnu11 -O2 -I. -Iuserspace/lib -ffreestanding -nostdlib


# --- Project Structure ---
BUILD_DIR = build
SRC_DIR = src
ISO_DIR = $(BUILD_DIR)/isodir
USER_DIR = userspace
MODULES_DIR = modules

# --- Files Path Logic ---
# Explicitly list all kernel C and ASM source files
K_C_SOURCES = $(shell find $(SRC_DIR)/kernel -name "*.c")
K_ASM_SOURCES = $(shell find $(SRC_DIR)/boot -name "*.asm" | grep -v "long_mode_entry.asm")

# Превращаем пути из src/ в build/
K_C_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(K_C_SOURCES))
K_ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(K_ASM_SOURCES))
K_OBJS = $(K_C_OBJS) $(K_ASM_OBJS)

KERNEL_NAME = KyroOS
KERNEL = build/$(KERNEL_NAME).elf
GAME_ELF = $(BUILD_DIR)/userspace/game.elf
HELLO_LKM = $(MODULES_DIR)/hello_lkm/hello.ko
LOGO_FILE = KyroOS_logo.png

# --- Main Targets ---
.PHONY: all
all: update_version $(KERNEL_NAME).iso

$(KERNEL_NAME).iso: $(KERNEL) $(GAME_ELF) $(HELLO_LKM) grub/grub.cfg $(LOGO_FILE)
	@echo "Creating $(KERNEL_NAME) bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@mkdir -p $(ISO_DIR)/boot/modules
	@cp $(KERNEL) $(ISO_DIR)/boot/$(KERNEL_NAME).elf
	@cp $(GAME_ELF) $(ISO_DIR)/boot/game.elf
	@cp $(HELLO_LKM) $(ISO_DIR)/boot/modules/hello.ko
	@cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@cp $(LOGO_FILE) $(ISO_DIR)/boot/$(LOGO_FILE)
	@grub-mkrescue -o $(KERNEL_NAME).iso $(ISO_DIR)

# --- Versioning ---
VERSION_FILE = src/include/version.h

update_version:
	@echo "Updating build number in $(VERSION_FILE)..."
	@BUILD_NUM_STR=$$(grep "KYROS_VERSION_BUILD" $(VERSION_FILE) | awk '{print $$3}'); \
	BUILD_NUM=$$(echo $$BUILD_NUM_STR | sed 's/^0*//'); \
	NEW_BUILD_NUM=$$(($$BUILD_NUM + 1)); \
	NEW_BUILD_NUM_FORMATTED=$$(printf "%04d" $$NEW_BUILD_NUM); \
	sed -i "s/#define KYROS_VERSION_BUILD $$BUILD_NUM_STR/#define KYROS_VERSION_BUILD $$NEW_BUILD_NUM_FORMATTED/" $(VERSION_FILE)

# --- Kernel Build ---
$(KERNEL): $(K_OBJS) linker.ld $(VERSION_FILE)
	@$(LD) -T linker.ld -o $@ $(K_OBJS) -m elf_x86_64

$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%.c
	@mkdir -p $(@D)
	@$(CC) $(K_CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot/%.o: $(SRC_DIR)/boot/%.asm
	@mkdir -p $(@D)
	@$(AS) $(NASMFLAGS) $< -o $@

# --- Userspace Build ---
$(GAME_ELF): $(BUILD_DIR)/userspace/game.o $(BUILD_DIR)/userspace/libkyros_gfx.a
	@$(LD) -T $(USER_DIR)/game/link.ld -o $@ $^ -L$(BUILD_DIR)/userspace

$(BUILD_DIR)/userspace/game.o: $(USER_DIR)/game/game.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/libkyros_gfx.a: $(BUILD_DIR)/userspace/kyros_gfx.o
	@$(AR) rcs $@ $< 

$(BUILD_DIR)/userspace/kyros_gfx.o: $(USER_DIR)/lib/kyros_gfx/kyros_gfx.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# --- Module Build ---
.PHONY: modules
modules: $(HELLO_LKM)

$(HELLO_LKM):
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm

# --- Execution and Cleanup ---
.PHONY: run
run: $(KERNEL_NAME).iso
	@echo "Launching QEMU from Windows..."
	@cmd.exe /c qemu-system-x86_64.exe -cdrom $(KERNEL_NAME).iso -serial stdio

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(KERNEL_NAME).iso
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm clean 2>/dev/null || true