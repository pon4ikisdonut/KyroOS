#ifndef KYROS_GFX_H
#define KYROS_GFX_H

#include <stdint.h>

// Event definitions mirrored from kernel/event.h for userspace
typedef enum {
    GFX_EVENT_NONE,
    GFX_EVENT_KEY_DOWN,
    GFX_EVENT_KEY_UP,
    // ... other events
} gfx_event_type_t;

typedef struct {
    gfx_event_type_t type;
    int32_t data1; // For keyboard: the character code
    int32_t data2; // For keyboard: the raw scancode
} gfx_event_t;


int gfx_init();
void gfx_draw_pixel(int x, int y, uint32_t color);
void gfx_draw_rect(int x, int y, int width, int height, uint32_t color);
void gfx_flip(); // No-op for now as we draw directly
int gfx_poll_event(gfx_event_t* event);
uint32_t gfx_get_width();
uint32_t gfx_get_height();


#endif // KYROS_GFX_H
