#ifndef FB_H
#define FB_H

#include <stdint.h>
#include "multiboot2.h"

typedef struct {
    uint32_t* address; // Pointer to framebuffer memory
    uint32_t width;
    uint32_t height;
    uint16_t pitch;
    uint8_t bpp;
} fb_info_t;

void fb_init(uint32_t multiboot_info_addr);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear_screen(uint32_t color);
void fb_draw_char(char c, uint32_t x, uint32_t y, uint32_t fg_color, uint32_t bg_color);
const fb_info_t* fb_get_info();

#endif // FB_H
