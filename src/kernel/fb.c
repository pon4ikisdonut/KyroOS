#include "fb.h"
#include "font.h"
#include "log.h"
#include <stddef.h> // for NULL

static fb_info_t fb_info;

// Basic 8x16 font for ASCII characters
// This is a minimal font for demonstration. A full font would be much larger.
const uint8_t default_font[256][FONT_HEIGHT] = {
    // Basic characters (e.g., 'A', 'B', 'C', '!', '?')
    // For now, let's just define a solid block for any character to test drawing
    [0 ... 255] = { // Default all chars to a block
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    ['A'] = {
        0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42,
        0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    ['!'] = {
        0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
};

void fb_init(uint32_t multiboot_info_addr) {
    struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)find_multiboot_tag(multiboot_info_addr, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

    if (!fb_tag) {
        panic("Framebuffer not found!", NULL);
        return;
    }

    fb_info.address = (uint32_t*)(uint64_t)fb_tag->common.framebuffer_addr;
    fb_info.width = fb_tag->common.framebuffer_width;
    fb_info.height = fb_tag->common.framebuffer_height;
    fb_info.pitch = fb_tag->common.framebuffer_pitch;
    fb_info.bpp = fb_tag->common.framebuffer_bpp;

    if (fb_info.bpp != 32) {
        panic("Framebuffer is not 32-bpp!", NULL);
        return;
    }

    klog(LOG_INFO, "Framebuffer initialized.");
    fb_clear_screen(0x00101030); // Dark blue background
}

const fb_info_t* fb_get_info() {
    return &fb_info;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_info.width || y >= fb_info.height) {
        return;
    }
    // Calculate the offset in the linear framebuffer
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
    const uint8_t* glyph = default_font[(int)c];

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
