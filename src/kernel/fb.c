#include "fb.h"
#include "font.h"
#include "log.h"
#include "multiboot2.h" // Needed for the framebuffer tag struct
#include <stddef.h> // for NULL

static fb_info_t fb_info;

// Basic 8x16 font for ASCII characters
// A full font would be much larger. This is just for demonstration.
const uint8_t default_font[256][FONT_HEIGHT] = {
    // Only defining a few characters. C automatically zero-initializes the rest.
    ['A'] = { 0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    ['B'] = { 0x00, 0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    ['C'] = { 0x00, 0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    ['!'] = { 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    // A block for any undefined character
    [0] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
};

void fb_init(uint32_t multiboot_info_addr) {
    struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)find_multiboot_tag(multiboot_info_addr, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

    if (!fb_tag) {
        // Cannot use klog/panic here as the framebuffer isn't initialized.
        // Halt in a loop.
        for(;;);
    }

    fb_info.address = (uint32_t*)(uint64_t)fb_tag->common.framebuffer_addr;
    fb_info.width = fb_tag->common.framebuffer_width;
    fb_info.height = fb_tag->common.framebuffer_height;
    fb_info.pitch = fb_tag->common.framebuffer_pitch;
    fb_info.bpp = fb_tag->common.framebuffer_bpp;

    if (fb_info.bpp != 32) {
        // Cannot use klog/panic.
        for(;;);
    }
    
    // Do not log from here, as log_init() hasn't been called.
    // kmain will log after calling this.
    fb_clear_screen(0x00101030); // Dark blue background
}

const fb_info_t* fb_get_info() {
    return &fb_info;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_info.width || y >= fb_info.height) {
        return;
    }
    uint64_t offset = (y * fb_info.pitch) + (x * (fb_info.bpp / 8));
    *(uint32_t*)((uint64_t)fb_info.address + offset) = color;
}

void fb_clear_screen(uint32_t color) {
    for (uint32_t y = 0; y < fb_info.height; y++) {
        for (uint32_t x = 0; x < fb_info.width; x++) {
            fb_put_pixel(x, y, color);
        }
    }
}

void fb_draw_char(char c, uint32_t x, uint32_t y, uint32_t fg_color, uint32_t bg_color) {
    // Use the block for any character that is not defined
    const uint8_t* glyph = (c > 0 && default_font[(int)c][0] != 0) ? default_font[(int)c] : default_font[0];

    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            if ((glyph[row] >> (7 - col)) & 1) {
                fb_put_pixel(x + col, y + row, fg_color);
            } else {
                fb_put_pixel(x + col, y + row, bg_color);
            }
        }
    }
}