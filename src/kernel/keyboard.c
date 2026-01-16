#include "keyboard.h"
#include "event.h"
#include "isr.h"
#include "log.h" // Added
#include "port_io.h"
#include <stdbool.h>

// Basic US QWERTY scancode to ASCII map
// Does not handle shift, caps lock, etc. for now
static const unsigned char kbd_us[128] = {
    0,    27,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
    '\n', 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,    '*',
    0,    ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0,    0,    0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,    0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static bool ctrl_pressed = false;

bool keyboard_is_ctrl_pressed() {
    return ctrl_pressed;
}

static void keyboard_handler(struct registers *regs) {
  (void)regs; // Suppress unused parameter warning

  uint8_t scancode = inb(0x60);
  event_t event;

  // Handle modifier keys
  if (scancode == 0x1D) { // Left Ctrl pressed
    ctrl_pressed = true;
    return;
  } else if (scancode == 0x9D) { // Left Ctrl released
    ctrl_pressed = false;
    return;
  }

  if (scancode & 0x80) { // Key release
    event.type = EVENT_KEY_UP;
    event.data1 = kbd_us[scancode & 0x7F];
  } else { // Key press
    event.type = EVENT_KEY_DOWN;
    event.data1 = kbd_us[scancode];
  }
  event.data2 = scancode; // Store raw scancode in data2
  event_push(event);
}

void keyboard_init() { register_irq_handler(1, keyboard_handler); }