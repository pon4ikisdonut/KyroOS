#include "panic_screen.h"
#include "fb.h"
#include "font.h"
#include "kstring.h"
#include "version.h"
#include "isr.h"
#include "port_io.h"
#include "vfs.h"
#include "heap.h"
#include "log.h" // For klog
#include "isr.h"
#include "pmm.h"

#define PANIC_BG_COLOR 0xFFFF0000
#define PANIC_FG_COLOR 0xFFFFFFFF
#define INFO_TEXT_COLOR 0xFFDDDDDD // Lighter color for info text

static void panic_get_cpu_brand(char* buf);

// Global storage for quotes, now unused but kept for potential future use
static char **panic_quotes = NULL;
static size_t num_panic_quotes = 0;

// Helper to get CPU brand string, adapted from shell.c
static void panic_get_cpu_brand(char* buf) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid" : "=a"(eax) : "a"(0x80000000));
    if (eax < 0x80000004) {
        strncpy(buf, "Generic x86_64", 48);
        return;
    }
    uint32_t* ptr = (uint32_t*)buf;
    for (uint32_t i = 0; i < 3; i++) {
        __asm__ __volatile__("cpuid"
                            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                            : "a"(0x80000002 + i));
        ptr[i * 4 + 0] = eax;
        ptr[i * 4 + 1] = ebx;
        ptr[i * 4 + 2] = ecx;
        ptr[i * 4 + 3] = edx;
    }
    buf[47] = '\0'; // Ensure null termination
    // Trim leading spaces
    char* start = buf;
    while (*start == ' ') start++;
    if (start != buf) {
        char* dst = buf;
        while (*start) *dst++ = *start++;
        *dst = '\0';
    }
}



static void panic_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    fb_draw_char(c, x, y, fg, bg);
}

static void panic_print_str(int *x, int *y, const char *s, uint32_t fg, uint32_t bg) {
    const fb_info_t *fb_info = fb_get_info();
    if (!fb_info) return;

    int start_x = *x;
    int max_x = fb_info->width;

    while (*s) {
        char c = *s++;
        if (c == '\n') {
            *y += FONT_HEIGHT;
            *x = start_x;
        } else {
            if (*x + FONT_WIDTH > max_x) {
                *y += FONT_HEIGHT;
                *x = start_x;
            }
            panic_draw_char(*x, *y, c, fg, bg);
            *x += FONT_WIDTH;
        }
    }
}

static void print_page_fault_error(int *x, int *y, uint64_t error_code, uint32_t fg, uint32_t bg) {
    char buf[100];
    ksprintf(buf, "Page Fault Error Code: %016lx", error_code);
    panic_print_str(x, y, buf, fg, bg);
    *y += FONT_HEIGHT; *x = 20;

    if (error_code & 0x1) panic_print_str(x, y, "  - Protection Violation (P)", fg, bg);
    else panic_print_str(x, y, "  - Page Not Present (P)", fg, bg);
    *y += FONT_HEIGHT; *x = 20;

    if (error_code & 0x2) panic_print_str(x, y, "  - Write Operation (W/R)", fg, bg);
    else panic_print_str(x, y, "  - Read Operation (W/R)", fg, bg);
    *y += FONT_HEIGHT; *x = 20;

    if (error_code & 0x4) panic_print_str(x, y, "  - User Mode (U/S)", fg, bg);
    else panic_print_str(x, y, "  - Supervisor Mode (U/S)", fg, bg);
    *y += FONT_HEIGHT; *x = 20;

    if (error_code & 0x8) panic_print_str(x, y, "  - Reserved Bit Set (RSVD)", fg, bg);
    *y += FONT_HEIGHT; *x = 20;

    if (error_code & 0x10) panic_print_str(x, y, "  - Instruction Fetch (I/D)", fg, bg);
    *y += FONT_HEIGHT; *x = 20;
}

static void print_stack_trace(int *x, int *y, uint64_t rbp, uint32_t fg, uint32_t bg) {
    *y += FONT_HEIGHT;
    *x = 20;
    panic_print_str(x, y, "Stack Trace (RBP chain):", fg, bg);
    *y += FONT_HEIGHT;
    *x = 20;

    uint64_t *frame = (uint64_t *)rbp;
    for (int i = 0; i < 10 && frame; i++) { // Limit to 10 frames
        uint64_t rip = frame[1];
        if (rip == 0) break; // End of stack
        char buf[30];
        ksprintf(buf, "  0x%016lx", rip);
        panic_print_str(x, y, buf, fg, bg);
        *y += FONT_HEIGHT;
        *x = 20;
        frame = (uint64_t *)frame[0]; // Move to previous frame
    }
}


void panic_screen_init(void) {
    // This function is now a stub, as the quote feature was removed.
    // Kept for compatibility with calls in kmain.
    return;
}


void panic_screen_show(const char *message, struct registers *regs) {
    // All drawing operations will now use the backbuffer if it's available,
    // and we will flush it once at the end. This is safer.
    fb_clear(PANIC_BG_COLOR);

    const fb_info_t *fb_info = fb_get_info();
    if (!fb_info) { // Fallback if no framebuffer
        for(;;) { __asm__ __volatile__("cli; hlt"); }
    }

    int x = 20;
    int y = 20;
    
    // --- Top Left: Panic Message ---
    panic_print_str(&x, &y, "!!! KERNEL PANIC !!!", PANIC_FG_COLOR, PANIC_BG_COLOR);
    y += FONT_HEIGHT * 2;
    x = 20;

    panic_print_str(&x, &y, "Message: ", PANIC_FG_COLOR, PANIC_BG_COLOR);
    panic_print_str(&x, &y, message, PANIC_FG_COLOR, PANIC_BG_COLOR);
    y += FONT_HEIGHT * 2;
    x = 20;

    // --- Top Right: System Information ---
    char info_buf[128];
    int right_col_x = fb_info->width - 350;
    int temp_x = right_col_x;
    int temp_y = 20;

    panic_print_str(&temp_x, &temp_y, "--- System Info ---", INFO_TEXT_COLOR, PANIC_BG_COLOR);
    temp_y += FONT_HEIGHT; temp_x = right_col_x;

    ksprintf(info_buf, "KyroOS Version: %s (Build %s)", KYROOS_VERSION_STRING, KYROOS_VERSION_BUILD);
    panic_print_str(&temp_x, &temp_y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
    temp_y += FONT_HEIGHT; temp_x = right_col_x;

    char cpu_brand[48];
    panic_get_cpu_brand(cpu_brand);
    ksprintf(info_buf, "CPU: %s", cpu_brand);
    panic_print_str(&temp_x, &temp_y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
    temp_y += FONT_HEIGHT; temp_x = right_col_x;

    uint64_t total_mem = pmm_get_total_memory();
    uint64_t used_mem = pmm_get_used_memory();
    ksprintf(info_buf, "RAM: %d / %d MB used", used_mem / 1024 / 1024, total_mem / 1024 / 1024);
    panic_print_str(&temp_x, &temp_y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
    temp_y += FONT_HEIGHT; temp_x = right_col_x;

    ksprintf(info_buf, "Display: %dx%d @ %dbpp", fb_info->width, fb_info->height, fb_info->bpp);
    panic_print_str(&temp_x, &temp_y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
    
    // Reset Y to be below the panic message for the main content
    y = 20 + (FONT_HEIGHT * 5);
    x = 20;

    // --- Last Log Messages ---
    panic_print_str(&x, &y, "--- Last 5 Log Messages ---", INFO_TEXT_COLOR, PANIC_BG_COLOR);
    y += FONT_HEIGHT; x = 20;
    
    const int num_logs = 5;
    char log_entries[num_logs][LOG_MESSAGE_MAX_LEN];
    int count = 0;
    log_get_entries((char*)log_entries, &count, num_logs);

    for (int i = 0; i < count; i++) {
        panic_print_str(&x, &y, log_entries[i], PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = 20;
    }
    y += FONT_HEIGHT; x = 20;

    // --- Detailed Error Info ---
    if (regs) {
        if (regs->int_no == 14) { // Page Fault
            uint64_t cr2;
            __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
            ksprintf(info_buf, "Faulting Address (CR2): %016lx", cr2);
            panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
            y += FONT_HEIGHT; x = 20;
            print_page_fault_error(&x, &y, regs->err_code, PANIC_FG_COLOR, PANIC_BG_COLOR);
        }

        panic_print_str(&x, &y, "--- Registers ---", INFO_TEXT_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = 20;

        int reg_col_width = (fb_info->width / 2) - 40;
        int original_x = x;

        ksprintf(info_buf, "RAX: %016lx", regs->rax); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "RBX: %016lx", regs->rbx); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "RCX: %016lx", regs->rcx); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "RDX: %016lx", regs->rdx); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "RSI: %016lx", regs->rsi); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "RDI: %016lx", regs->rdi); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "RBP: %016lx", regs->rbp); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "RSP: %016lx", regs->rsp); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "R8:  %016lx", regs->r8); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "R9:  %016lx", regs->r9); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "R10: %016lx", regs->r10); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "R11: %016lx", regs->r11); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "R12: %016lx", regs->r12); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "R13: %016lx", regs->r13); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "R14: %016lx", regs->r14); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "R15: %016lx", regs->r15); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "RIP: %016lx", regs->rip); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        x = original_x + reg_col_width; ksprintf(info_buf, "RFL: %016lx", regs->rflags); panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;
        
        ksprintf(info_buf, "CS:  %04lx SS: %04lx", (uint64_t)regs->cs, (uint64_t)regs->ss);
        panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT; x = original_x;

        ksprintf(info_buf, "INT: %02x, ERR: %04lx", regs->int_no, regs->err_code);
        panic_print_str(&x, &y, info_buf, PANIC_FG_COLOR, PANIC_BG_COLOR);
        y += FONT_HEIGHT * 2; x = 20;

        print_stack_trace(&x, &y, regs->rbp, PANIC_FG_COLOR, PANIC_BG_COLOR);
    } else {
        panic_print_str(&x, &y, "No register state available.", PANIC_FG_COLOR, PANIC_BG_COLOR);
    }

    // --- Bottom Right: QR Code Placeholder ---
    temp_x = fb_info->width - (35 * FONT_WIDTH); 
    temp_y = fb_info->height - (3 * FONT_HEIGHT);
    panic_print_str(&temp_x, &temp_y, "[QR Code Placeholder]", INFO_TEXT_COLOR, PANIC_BG_COLOR);
    temp_y += FONT_HEIGHT;
    temp_x = fb_info->width - (35 * FONT_WIDTH);
    panic_print_str(&temp_x, &temp_y, "kyroos.pp.ua/kernel-panic", INFO_TEXT_COLOR, PANIC_BG_COLOR);
    
    // Make sure everything is drawn to the screen.
    fb_flush();

    // Halt the system
    for(;;) { __asm__ __volatile__("cli; hlt"); }
}