#include "mouse.h"
#include "isr.h"
#include "log.h"
#include "port_io.h"
#include "fb.h" // For getting screen dimensions

#define KBC_STATUS_PORT 0x64
#define KBC_CMD_PORT    0x64
#define KBC_DATA_PORT   0x60
#define MOUSE_DATA_PORT 0x60

static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

// Helper to wait for the keyboard controller to be ready
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { // Wait for input buffer to be empty
        while (timeout-- && (inb(KBC_STATUS_PORT) & 2));
    } else { // Wait for output buffer to be full
        while (timeout-- && !(inb(KBC_STATUS_PORT) & 1));
    }
}

// Helper to write to the mouse device
static void mouse_write(uint8_t data) {
    mouse_wait(0);
    outb(KBC_CMD_PORT, 0xD4); // Tell KBC we are sending a command to the mouse
    mouse_wait(0);
    outb(KBC_DATA_PORT, data);
}

// Helper to read from the mouse device
static uint8_t mouse_read() {
    mouse_wait(1);
    return inb(KBC_DATA_PORT);
}

// The main IRQ12 handler
void mouse_handler(struct registers regs) {
    uint8_t status = inb(KBC_CMD_PORT);
    if (!(status & 0x20)) return; // Check if data is from mouse

    switch(mouse_cycle) {
        case 0:
            mouse_byte[0] = inb(MOUSE_DATA_PORT);
            // Check for valid first byte (bit 3 should be 1)
            if (mouse_byte[0] & 0x08) {
                mouse_cycle++;
            }
            break;
        case 1:
            mouse_byte[1] = inb(MOUSE_DATA_PORT);
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = inb(MOUSE_DATA_PORT);
            mouse_cycle = 0;

            // We have a full packet, process it
            int8_t delta_x = mouse_byte[1];
            int8_t delta_y = mouse_byte[2];

            // Handle Y-axis inversion
            if (mouse_byte[0] & 0x20) {
                delta_y = -delta_y;
            }
            // Handle X-axis inversion/sign
            if (mouse_byte[0] & 0x10) {
                delta_x = -delta_x;
            }

            mouse_x += delta_x;
            mouse_y -= delta_y; // Y is typically inverted

            // Clamp to screen dimensions
            const fb_info_t* info = fb_get_info();
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= info->width) mouse_x = info->width - 1;
            if (mouse_y >= info->height) mouse_y = info->height - 1;

            // TODO: Handle button presses from mouse_byte[0]
            break;
    }
}

void mouse_init() {
    mouse_wait(0);
    outb(KBC_CMD_PORT, 0xA8); // Enable auxiliary device (mouse)

    // Enable IRQ12
    mouse_wait(0);
    outb(KBC_CMD_PORT, 0x20); // Command to get Compaq status byte
    mouse_wait(1);
    uint8_t status = inb(KBC_DATA_PORT);
    status |= 2; // Set bit 1 to enable IRQ12
    status &= ~0x20; // Clear bit 5 to disable mouse clock
    mouse_wait(0);
    outb(KBC_CMD_PORT, 0x60); // Command to set Compaq status byte
    mouse_wait(0);
    outb(KBC_DATA_PORT, status);

    // Set defaults and enable mouse
    mouse_write(0xF6); // Set defaults
    mouse_read();
    mouse_write(0xF4); // Enable data reporting
    mouse_read();

    register_irq_handler(12, mouse_handler);
    klog(LOG_INFO, "PS/2 Mouse initialized.");
}

void mouse_get_position(int32_t* x, int32_t* y) {
    *x = mouse_x;
    *y = mouse_y;
}
