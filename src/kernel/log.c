#include "log.h"
#include "isr.h"
#include "fb.h"
#include "font.h"
#include <stdint.h>
#include <string.h> // For memcpy

// Console state for framebuffer
static uint32_t console_x = 0;
static uint32_t console_y = 0;
static uint32_t fg_color = 0xFFFFFFFF; // White
static uint32_t bg_color = 0x00101030; // Dark Blue

// Helper for scrolling the framebuffer
static void console_scroll() {
    const fb_info_t* info = fb_get_info();
    uint32_t line_size = info->pitch;
    uint32_t scroll_height = info->height - FONT_HEIGHT;
    
    // Move all lines up by one character height
    memcpy((void*)info->address, (void*)(info->address + line_size * FONT_HEIGHT), scroll_height * line_size);
    
    // Clear the last line
    uint32_t* last_line = (uint32_t*)(info->address + scroll_height * line_size);
    for (uint32_t i = 0; i < (info->height - scroll_height) * info->width; i++) {
        last_line[i] = bg_color;
    }
    
    console_y--;
}

void klog_putchar(char c) {
    const fb_info_t* info = fb_get_info();
    if (!info || !info->address) return; // Can't draw if framebuffer is not ready

    if (c == '\n') {
        console_x = 0;
        console_y++;
    } else {
        fb_draw_char(c, console_x * FONT_WIDTH, console_y * FONT_HEIGHT, fg_color, bg_color);
        console_x++;
    }

    if (console_x * FONT_WIDTH >= info->width) {
        console_x = 0;
        console_y++;
    }
    if (console_y * FONT_HEIGHT >= info->height) {
        console_scroll();
    }
}

void klog_print_str(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        klog_putchar(str[i]);
    }
}

void klog_print_hex(uint64_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[19] = "0x";
    int i = 17;
    buffer[18] = '\0';

    if (val == 0) {
        buffer[2] = '0';
        buffer[3] = '\0';
        klog_print_str(buffer);
        return;
    }
    
    while (val > 0) {
        buffer[i--] = hex_chars[val % 16];
        val /= 16;
    }
    klog_print_str(&buffer[i + 1]);
}

void log_init() {
    console_x = 0;
    console_y = 0;
    // Clearing of screen is now handled in fb_init
}

void klog(log_level_t level, const char* message) {
    uint32_t old_fg = fg_color;
    
    switch (level) {
        case LOG_INFO:
            fg_color = 0xFFFFFFFF; // White
            klog_print_str("[INFO] ");
            break;
        case LOG_WARN:
            fg_color = 0xFFFFFF00; // Yellow
            klog_print_str("[WARN] ");
            break;
        case LOG_ERROR:
            fg_color = 0xFFFF0000; // Red
            klog_print_str("[ERROR] ");
            break;
        case LOG_PANIC:
            fg_color = 0xFFFFFFFF; // White on Red
            bg_color = 0xFFFF0000;
            klog_print_str("[PANIC] ");
            break;
    }
    
    klog_print_str(message);
    klog_putchar('\n');
    fg_color = old_fg;
    if(level == LOG_PANIC) bg_color = 0x00101030; // Reset bg color
}

void panic(const char* message, struct registers* regs) {
    disable_interrupts(); // Critical: disable interrupts during panic

    uint32_t old_bg = bg_color;
    bg_color = 0xFF800000; // Dark Red background
    fg_color = 0xFFFFFFFF; // White text
    
    fb_clear_screen(bg_color);
    console_x = 0;
    console_y = 0;

    klog_print_str("!!! KERNEL PANIC !!!\n"); // Linus Torvalds, hello, when will you add beautiful panic screens?
    klog_print_str("Message: ");
    klog_print_str(message);
    klog_putchar('\n');

    if (regs) {
        klog_print_str("Interrupt: "); klog_print_hex(regs->int_no); klog_putchar('\n');
        klog_print_str("Error Code: "); klog_print_hex(regs->err_code); klog_putchar('\n');
        klog_print_str("RIP: "); klog_print_hex(regs->rip); klog_putchar('\n');
        klog_print_str("CS: "); klog_print_hex(regs->cs); klog_putchar('\n');
        klog_print_str("RFLAGS: "); klog_print_hex(regs->rflags); klog_putchar('\n');
        klog_print_str("RSP: "); klog_print_hex(regs->rsp); klog_putchar('\n');
        klog_print_str("SS: "); klog_print_hex(regs->ss); klog_putchar('\n');
    }

    // Halt the CPU indefinitely
    for (;;) {
        asm volatile ("hlt");
    }
    bg_color = old_bg; // Will never be reached
}