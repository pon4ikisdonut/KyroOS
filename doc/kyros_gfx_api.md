# kyros_gfx Library API

This is a minimal userspace graphics and input library for KyroOS.

## Usage

Include `kyros_gfx.h` in your userspace application and link against `libkyros_gfx.a`.

## Functions

### `int gfx_init()`
Initializes the graphics library. It must be called before any other `gfx_` function. It communicates with the kernel via syscall to get the framebuffer information.

- **Returns**: `0` on success, `-1` on failure.

---

### `void gfx_draw_pixel(int x, int y, uint32_t color)`
Draws a single pixel on the screen.

- **`x`**: The x-coordinate.
- **`y`**: The y-coordinate.
- **`color`**: The 32-bit color of the pixel (e.g., `0xAARRGGBB`).

---

### `void gfx_draw_rect(int x, int y, int width, int height, uint32_t color)`
Draws a filled rectangle on the screen.

- **`x`, `y`**: Top-left corner coordinates.
- **`width`, `height`**: Dimensions of the rectangle.
- **`color`**: The 32-bit fill color.

---

### `int gfx_poll_event(gfx_event_t* event)`
Polls for the next input event from the kernel's queue. This function is non-blocking.

- **`event`**: A pointer to a `gfx_event_t` struct that will be filled with event data if one is available.
- **Returns**: `1` if an event was retrieved, `0` if the event queue was empty.

The `gfx_event_t` struct provides the event type and associated data (e.g., key scancode).

---

### `uint32_t gfx_get_width()`
- **Returns**: The width of the screen in pixels.

---

### `uint32_t gfx_get_height()`
- **Returns**: The height of the screen in pixels.
