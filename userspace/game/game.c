#include "../lib/kyros_gfx/kyros_gfx.h"

void _start() {
    if (gfx_init() != 0) {
        // Failed to init graphics, hang
        for(;;);
    }
    
    int x = 100;
    int y = 100;
    int size = 20;

    int running = 1;
    while(running) {
        // 1. Process Input
        gfx_event_t event;
        while (gfx_poll_event(&event)) {
            if (event.type == GFX_EVENT_KEY_DOWN) {
                switch (event.data1) {
                    case 'w': y -= 5; break;
                    case 's': y += 5; break;
                    case 'a': x -= 5; break;
                    case 'd': x += 5; break;
                    case 'q': running = 0; break;
                }
            }
        }
        
        // Clamp position
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > gfx_get_width() - size) x = gfx_get_width() - size;
        if (y > gfx_get_height() - size) y = gfx_get_height() - size;

        // 2. Draw scene
        gfx_draw_rect(0, 0, gfx_get_width(), gfx_get_height(), 0x00000000); // Clear screen (black)
        gfx_draw_rect(x, y, size, size, 0xFF00FF00); // Draw green square
        
        // No vsync, just a busy loop for delay
        for(volatile int i = 0; i < 500000; i++);
    }
    
    // Exit (syscall 0)
    asm volatile ("mov $0, %rax; int $0x80");
}
