# KyroOS Build System (Optimized for WSL + Windows QEMU)

# --- Toolchain Setup ---
# В Ubuntu/WSL пакет называется x86_64-linux-gnu, он отлично подходит
# при использовании флагов -ffreestanding и -nostdlib
TARGET = x86_64-linux-gnu
CC = $(TARGET)-gcc
AR = $(TARGET)-ar
AS = nasm
LD = $(TARGET)-ld

# Kernel compilation flags
CFLAGS := \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-pie \
    -fno-pic \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-80387 \
# --- Compilation Flags ---
# Optimized for Kernel Development
K_CFLAGS = -Wall -Wextra -std=c11 -ffreestanding -O2 -Isrc/include \
           -mcmodel=kernel -mno-red-zone -m64 -nostdlib -fno-stack-protector \
           -mno-sse -mno-sse2 -mno-mmx -mno-80387 -fno-pic -fno-pie
NASMFLAGS = -f elf64

# Userspace flags
U_CFLAGS = -Wall -Wextra -std=gnu11 -O2 -I. -Iuserspace/lib -I$(SRC_DIR)/tools -I$(SRC_DIR)/include -ffreestanding -nostdlib


# --- Project Structure ---
BUILD_DIR = build
SRC_DIR = src
ISO_DIR = $(BUILD_DIR)/isodir
USER_DIR = userspace
MODULES_DIR = modules

# --- Files Path Logic ---
K_OBJS = \
	$(BUILD_DIR)/boot/boot.o \
	$(BUILD_DIR)/boot/gdt_flush.o \
	$(BUILD_DIR)/boot/isr_stubs.o \
	$(BUILD_DIR)/boot/long_mode_entry.o \
	$(BUILD_DIR)/boot/switch.o \
	$(BUILD_DIR)/boot/userspace_exit_stub.o \
	$(BUILD_DIR)/kernel/ac97.o \
	$(BUILD_DIR)/kernel/arp.o \
	$(BUILD_DIR)/kernel/audio.o \
	$(BUILD_DIR)/kernel/crypto.o \
	$(BUILD_DIR)/kernel/deviceman.o \
	$(BUILD_DIR)/kernel/dhcp.o \
	$(BUILD_DIR)/kernel/e1000.o \
	$(BUILD_DIR)/kernel/elf.o \
	$(BUILD_DIR)/kernel/event.o \
	$(BUILD_DIR)/kernel/epstein.o \
	$(BUILD_DIR)/kernel/fb.o \
	$(BUILD_DIR)/kernel/font.o \
	$(BUILD_DIR)/kernel/gdt.o \
	$(BUILD_DIR)/kernel/gui.o \
	$(BUILD_DIR)/kernel/heap.o \
	$(BUILD_DIR)/kernel/icmp.o \
	$(BUILD_DIR)/kernel/ide.o \
	$(BUILD_DIR)/kernel/idt.o \
	$(BUILD_DIR)/kernel/ip.o \
	$(BUILD_DIR)/kernel/isr.o \
	$(BUILD_DIR)/kernel/kernel.o \
	$(BUILD_DIR)/kernel/keyboard.o \
	$(BUILD_DIR)/kernel/kyrofs.o \
	$(BUILD_DIR)/kernel/lkm.o \
	$(BUILD_DIR)/kernel/log.o \
	$(BUILD_DIR)/kernel/mouse.o \
	$(BUILD_DIR)/kernel/null_pci_driver.o \
	$(BUILD_DIR)/kernel/pci.o \
	$(BUILD_DIR)/kernel/pmm.o \
	$(BUILD_DIR)/kernel/panic_screen.o \
	$(BUILD_DIR)/kernel/scheduler.o \
	$(BUILD_DIR)/kernel/shell.o \
	$(BUILD_DIR)/kernel/socket.o \
	$(BUILD_DIR)/kernel/string.o \
	$(BUILD_DIR)/kernel/syscall.o \
	$(BUILD_DIR)/kernel/tcp.o \
	$(BUILD_DIR)/kernel/thread.o \
	$(BUILD_DIR)/kernel/tss.o \
	$(BUILD_DIR)/kernel/udp.o \
	$(BUILD_DIR)/kernel/userspace.o \
	$(BUILD_DIR)/kernel/vfs.o \
	$(BUILD_DIR)/kernel/vmm.o \
	$(BUILD_DIR)/kernel/fs_disk.o \
	$(BUILD_DIR)/kernel/fs_disk_vfs.o


KERNEL_NAME = kyroos
KERNEL = build/$(KERNEL_NAME).elf
GAME_ELF = $(BUILD_DIR)/bin/game
TUI_INSTALLER_ELF = $(BUILD_DIR)/bin/tui_installer
INSTALLER_ELF = $(BUILD_DIR)/bin/installer
HELLO_LKM = $(MODULES_DIR)/hello_lkm/hello.ko
LOGO_FILE = kyroos_logo.png

# --- Main Targets ---
.PHONY: all
all: update_version tools userspace $(KERNEL_NAME).iso

$(KERNEL_NAME).iso: $(KERNEL) $(HELLO_LKM) limine.conf
	@echo "Creating $(KERNEL_NAME).iso (Build $(shell grep KYROOS_VERSION_BUILD $(VERSION_FILE) | awk '{print $$3}' | tr -d '"'))"
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/limine $(ISO_DIR)/bin $(ISO_DIR)/modules $(ISO_DIR)/etc
	@cp $(KERNEL) $(ISO_DIR)/kernel.elf
	@cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	@cp $(GAME_ELF) $(ISO_DIR)/bin/
	@cp $(TUI_INSTALLER_ELF) $(ISO_DIR)/bin/installer
	@cp $(COREUTILS_BINS) $(KPM_BIN) $(TEST_APP_BIN) $(ISO_DIR)/bin/
	@cp $(HELLO_LKM) $(ISO_DIR)/modules/
	@cp panic_quotes.txt $(ISO_DIR)/etc/
	@cp limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-bios-pxe.bin limine/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/
	@cp limine.conf limine/limine-bios.sys $(ISO_DIR)/
	@xorriso -as mkisofs -R -J -iso-level 3 -volid "KYROOS" -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        $(ISO_DIR) -o $(KERNEL_NAME).iso

# --- Versioning ---
VERSION_FILE = src/include/version.h

update_version:
	@echo "Updating build number in $(VERSION_FILE)..."
	@awk '/KYROOS_VERSION_BUILD/ {$$3 = sprintf("\"%04d\"", substr($$3, 2, length($$3)-2) + 1)} {print}' $(VERSION_FILE) > $(VERSION_FILE).tmp
	@mv $(VERSION_FILE).tmp $(VERSION_FILE)

# --- Kernel Build ---
$(KERNEL): $(BUILD_DIR)/kernel/libkyroos_kernel.a linker.ld $(VERSION_FILE)
	@$(LD) -T linker.ld -o $@ $(BUILD_DIR)/kernel/libkyroos_kernel.a -m elf_x86_64

$(BUILD_DIR)/kernel/libkyroos_kernel.a: $(K_OBJS)
	@$(AR) rcs $@ $^

$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%.c
	@mkdir -p $(@D)
	@$(CC) $(K_CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot/%.o: $(SRC_DIR)/boot/%.asm
	@mkdir -p $(@D)
	@$(AS) $(NASMFLAGS) $< -o $@

# --- Userspace Build ---
.PHONY: userspace
userspace: $(USER_LIBC_A) $(INSTALLER_ELF) $(TUI_INSTALLER_ELF) $(GAME_ELF)

$(GAME_ELF): $(BUILD_DIR)/userspace/game.o $(BUILD_DIR)/userspace/libkyroos_gfx.a $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T $(USER_DIR)/game/link.ld -o $@ $(BUILD_DIR)/userspace/game.o $(BUILD_DIR)/userspace/crt0.o -L$(BUILD_DIR)/userspace -lkyroos_gfx -lkyroos_user

$(BUILD_DIR)/userspace/game.o: $(USER_DIR)/game/game.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/libkyroos_gfx.a: $(BUILD_DIR)/userspace/kyroos_gfx.o
	@$(AR) rcs $@ $< 

$(BUILD_DIR)/userspace/kyroos_gfx.o: $(USER_DIR)/lib/kyroos_gfx/kyroos_gfx.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# TUI Installer
$(TUI_INSTALLER_ELF): $(BUILD_DIR)/userspace/tui_installer.o $(BUILD_DIR)/userspace/libtui.a $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T $(USER_DIR)/installer/link.ld -o $@ $(BUILD_DIR)/userspace/tui_installer.o $(BUILD_DIR)/userspace/crt0.o -L$(BUILD_DIR)/userspace -ltui -lkyroos_user

$(BUILD_DIR)/userspace/tui_installer.o: $(USER_DIR)/installer/installer.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/libtui.a: $(BUILD_DIR)/userspace/tui.o
	@$(AR) rcs $@ $< 

$(BUILD_DIR)/userspace/tui.o: $(USER_DIR)/lib/tui/tui.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# CLI Installer
$(INSTALLER_ELF): $(BUILD_DIR)/tools/installer/installer.o $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T src/tools/installer/link.ld -o $@ $(BUILD_DIR)/tools/installer/installer.o $(BUILD_DIR)/userspace/crt0.o -L$(BUILD_DIR)/userspace -lkyroos_user

$(BUILD_DIR)/tools/installer/installer.o: $(SRC_DIR)/tools/installer/installer.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

# --- Userspace Libc Build ---
USER_LIBC_SRC = $(shell find $(USER_DIR)/lib/libc -name "*.c")
USER_LIBC_OBJS = $(patsubst $(USER_DIR)/lib/libc/%.c, $(BUILD_DIR)/userspace/libc/%.o, $(USER_LIBC_SRC))
USER_LIBC_A = $(BUILD_DIR)/userspace/libkyroos_user.a

$(BUILD_DIR)/userspace/libc/%.o: $(USER_DIR)/lib/libc/%.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(USER_LIBC_A): $(USER_LIBC_OBJS)
	@$(AR) rcs $@ $^

# --- Module Build ---
.PHONY: modules
modules: $(HELLO_LKM)

$(HELLO_LKM):
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm

# --- Tools & CoreUtils Build ---
COREUTILS_SRC = $(shell find $(SRC_DIR)/tools/coreutils -name "*.c")
COREUTILS_OBJS = $(patsubst $(SRC_DIR)/tools/coreutils/%.c, $(BUILD_DIR)/tools/coreutils/%.o, $(COREUTILS_SRC))
COREUTILS_BINS = $(patsubst $(SRC_DIR)/tools/coreutils/%.c, $(BUILD_DIR)/bin/%, $(COREUTILS_SRC))

KPM_SRC = $(shell find $(SRC_DIR)/tools/kpm -name "*.c")
KPM_OBJS = $(patsubst $(SRC_DIR)/tools/kpm/%.c, $(BUILD_DIR)/tools/kpm/%.o, $(KPM_SRC))
KPM_BIN = $(BUILD_DIR)/bin/kpm

TEST_APP_SRC = $(SRC_DIR)/tools/test_app/test_app.c
TEST_APP_OBJ = $(BUILD_DIR)/tools/test_app/test_app.o
TEST_APP_BIN = $(BUILD_DIR)/bin/test_app

# Generic rule to compile userspace C files into object files
$(BUILD_DIR)/tools/coreutils/%.o: $(SRC_DIR)/tools/coreutils/%.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(BUILD_DIR)/tools/kpm/%.o: $(SRC_DIR)/tools/kpm/%.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@

$(BUILD_DIR)/tools/test_app/%.o: $(SRC_DIR)/tools/test_app/%.c
	@mkdir -p $(@D)
	@$(CC) $(U_CFLAGS) -c $< -o $@


.PHONY: tools
tools: $(USER_LIBC_A) $(COREUTILS_BINS) $(KPM_BIN) $(TEST_APP_BIN)

$(BUILD_DIR)/bin/%: $(BUILD_DIR)/tools/coreutils/%.o $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T src/tools/coreutils/link.ld -o $@ $^ -L$(BUILD_DIR)/userspace -lkyroos_user

$(KPM_BIN): $(KPM_OBJS) $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T src/tools/kpm/link.ld -o $@ $^ -L$(BUILD_DIR)/userspace -lkyroos_user

$(TEST_APP_BIN): $(TEST_APP_OBJ) $(BUILD_DIR)/userspace/crt0.o
	@mkdir -p $(@D)
	@$(LD) -T src/tools/coreutils/link.ld -o $@ $^ -L$(BUILD_DIR)/userspace -lkyroos_user

$(BUILD_DIR)/userspace/crt0.o: $(USER_DIR)/crt0.asm
	@mkdir -p $(@D)
	@$(AS) $(NASMFLAGS) $< -o $@

# --- Execution and Cleanup ---
.PHONY: run
run: clean all
	@echo "Launching QEMU from Windows..."
	@cmd.exe /c "C:\\Program Files\\qemu\\qemu-system-x86_64.exe" -cdrom $(KERNEL_NAME).iso -serial stdio -no-reboot -no-reboot -no-reboot

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(KERNEL_NAME).iso
	@$(MAKE) -C $(MODULES_DIR)/hello_lkm clean 2>/dev/null || true