#include "shell.h"
#include "event.h"
#include "fb.h"
#include "heap.h"
#include "kstring.h"
#include "log.h"
#include "pmm.h"
#include "scheduler.h"
#include "thread.h"
#include "version.h"
#include "vfs.h"
#include "keyboard.h"
#include "epstein.h" // For epstein global variable
#include "port_io.h"
#include "elf.h"
#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 256
#define SHELL_PROMPT "kyroos> "
#define HISTORY_SIZE 10

static char line_buffer[BUFFER_SIZE];
static int buffer_index = 0;

static char history[HISTORY_SIZE][BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;
static int current_history_view = -1;

static char cwd[256] = "/";

static void get_cpu_brand(char *buf) {
  uint32_t eax, ebx, ecx, edx;
  __asm__ __volatile__("cpuid" : "=a"(eax) : "a"(0x80000000));
  if (eax < 0x80000004) {
    strncpy(buf, "Generic x86_64 CPU", 48);
    return;
  }
  uint32_t *ptr = (uint32_t *)buf;
  for (uint32_t i = 0; i < 3; i++) {
    __asm__ __volatile__("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(0x80000002 + i));
    ptr[i * 4 + 0] = eax;
    ptr[i * 4 + 1] = ebx;
    ptr[i * 4 + 2] = ecx;
    ptr[i * 4 + 3] = edx;
  }
  buf[48] = '\0';
  char *start = buf;
  while (*start == ' ')
    start++;
  if (start != buf) {
    char *dst = buf;
    while (*start)
      *dst++ = *start++;
    *dst = '\0';
  }
}

static void shell_kyrofetch() {
  uint64_t total_bytes = pmm_get_total_memory();
  uint64_t used_bytes = pmm_get_used_memory();
  uint32_t total_mb = (uint32_t)(total_bytes / 1024 / 1024);
  uint32_t used_mb = (uint32_t)(used_bytes / 1024 / 1024);
  uint32_t mem_percent =
      (total_bytes > 0) ? (uint32_t)((used_bytes * 100ULL) / total_bytes) : 0;

  char buf[128];
  char cpu_brand[64];
  get_cpu_brand(cpu_brand);
  const fb_info_t *fb = fb_get_info();
  uint64_t ticks = timer_get_ticks();
  uint32_t total_sec = (uint32_t)(ticks / 100);
  uint32_t hrs = total_sec / 3600;
  uint32_t mins = (total_sec % 3600) / 60;
  uint32_t secs = total_sec % 60;

  klog_print_str("\n");
  klog_print_str("   _  ___              ____  ____\n");
  klog_print_str("  | |/ /_ _________   / __ \\/ __/\n");
  klog_print_str("  | ' / // / __/ _ \\ / /_/ /\\ \\  \n");
  klog_print_str("  |_|\\_\\_, /_/  \\___/ \\____/___/ \n");
  klog_print_str("      /___/                      \n");
  klog_print_str("\n");
  klog_print_str(" OS:      ");
  klog_print_str(KYROOS_VERSION_STRING);
  klog_print_str("\n");
  ksprintf(buf, " Kernel:  KyroOS %d.%d.%d (Build %s)", KYROOS_VERSION_MAJOR,
           KYROOS_VERSION_MINOR, KYROOS_VERSION_PATCH, KYROOS_VERSION_BUILD);
  klog_print_str(buf);
  klog_print_str("\n");
  if (hrs > 0)
    ksprintf(buf, " Uptime:  %d hours, %d mins, %d secs", hrs, mins, secs);
  else if (mins > 0)
    ksprintf(buf, " Uptime:  %d mins, %d secs", mins, secs);
  else
    ksprintf(buf, " Uptime:  %d secs", secs);
  klog_print_str(buf);
  klog_print_str("\n");
  ksprintf(buf, " CPU:     %s", cpu_brand);
  klog_print_str(buf);
  klog_print_str("\n");
  ksprintf(buf, " Display: %dx%d @ %dbpp", (int)fb->width, (int)fb->height,
           (int)fb->bpp);
  klog_print_str(buf);
  klog_print_str("\n");
  ksprintf(buf, " Memory:  %d MB / %d MB (%d%%)", used_mb, total_mb,
           mem_percent);
  klog_print_str(buf);
  klog_print_str("\n");
  klog_print_str(" Shell:   kyrooshell v0.2\n\n");
}

void shell_init() {
  buffer_index = 0;
  memset(line_buffer, 0, BUFFER_SIZE);
  klog_print_str("\nWelcome to KyroOS Shell!\n");
  klog_print_str("Type 'help' to see available commands.\n");
  klog_print_str("Type 'kyrofetch' for system info.\n");
  klog_print_str("Our website: https://kyroos.pp.ua/\n");
  klog_print_str("Discord: https://discord.gg/nSgmyadnbn\n\n");
  klog_print_str("user@kyroos:/> ");
}

static void add_to_history(const char *cmd) {
  if (strlen(cmd) == 0)
    return;
  if (history_count > 0 &&
      strcmp(history[(history_index) % HISTORY_SIZE], cmd) == 0)
    return;

  history_index = (history_index + 1) % HISTORY_SIZE;
  strncpy(history[history_index], cmd, BUFFER_SIZE);
  if (history_count < HISTORY_SIZE)
    history_count++;
  current_history_view = -1;
}

static void execute_command(char *line) {
  if (strlen(line) == 0)
    return;
  add_to_history(line);

  // Simple tokenization (cmd arg)
  char cmd[64];
  char arg[128];
  cmd[0] = '\0';
  arg[0] = '\0';

  char *space = strchr(line, ' ');
  if (space) {
    int cmd_len = space - line;
    if (cmd_len > 63)
      cmd_len = 63;
    strncpy(cmd, line, cmd_len);
    cmd[cmd_len] = '\0';
    strncpy(arg, space + 1, 127);
    arg[127] = '\0';
  } else {
    strncpy(cmd, line, 63);
    cmd[63] = '\0';
  }

  if (strcmp(cmd, "help") == 0) {
    klog_print_str(
        "Built-in: ls, cd, pwd, cat, mkdir, touch, rm, edit, kpm, clear, "
        "version, info, reboot, kyrofetch\n");
  } else if (strcmp(cmd, "pwd") == 0) {
    klog_print_str(cwd);
    klog_putchar('\n');
  } else if (strcmp(cmd, "cd") == 0) {
    if (strlen(arg) == 0) {
      strncpy(cwd, "/", 256);
      return;
    }

    // Handle .. specially to avoid creating literal ".." dirs
    if (strcmp(arg, "..") == 0) {
      if (strcmp(cwd, "/") != 0) {
        // Find last slash and truncate
        int last_slash = -1;
        for (int i = strlen(cwd) - 1; i >= 0; i--) {
          if (cwd[i] == '/') {
            last_slash = i;
            break;
          }
        }
        if (last_slash == 0) {
          cwd[1] = '\0'; // Go to root
        } else if (last_slash > 0) {
          cwd[last_slash] = '\0';
        }
      }
      return;
    }

    // Handle . (stay in current dir)
    if (strcmp(arg, ".") == 0)
      return;

    char full_path[256];
    if (arg[0] == '/') {
      strncpy(full_path, arg, 256);
    } else {
      if (strcmp(cwd, "/") == 0)
        ksprintf(full_path, "/%s", arg);
      else
        ksprintf(full_path, "%s/%s", cwd, arg);
    }

    vfs_node_t *node = vfs_resolve_path(vfs_root, full_path);
    if (node && (node->flags & VFS_DIRECTORY)) {
      strncpy(cwd, full_path, 256);
    } else {
      klog_print_str("No such directory: ");
      klog_print_str(arg);
      klog_putchar('\n');
    }
  } else if (strcmp(cmd, "ls") == 0) {
    char *path = (strlen(arg) > 0) ? arg : cwd;
    vfs_node_t *dir = vfs_resolve_path(vfs_root, path);
    if (!dir && path[0] != '/') {
      // Try relative
      char rel_path[256];
      if (strcmp(cwd, "/") == 0)
        ksprintf(rel_path, "/%s", path);
      else
        ksprintf(rel_path, "%s/%s", cwd, path);
      dir = vfs_resolve_path(vfs_root, rel_path);
    }

    if (dir && (dir->flags & VFS_DIRECTORY)) {
      struct dirent de; // Declare dirent on stack
      int i = 0;
      while (vfs_readdir(dir, i++, &de)) { // Pass address of de
        klog_print_str(de.name);
        klog_print_str("  ");
      }
      klog_putchar('\n');
    } else {
      klog_print_str("Directory not found.\n");
    }
  } else if (strcmp(cmd, "cat") == 0) {
    if (strlen(arg) == 0) {
      klog_print_str("Usage: cat <filename>\n");
      return;
    }
    vfs_node_t *node = vfs_resolve_path(vfs_root, arg);
    if (!node && arg[0] != '/') {
      char rel_path[256];
      if (strcmp(cwd, "/") == 0)
        ksprintf(rel_path, "/%s", arg);
      else
        ksprintf(rel_path, "%s/%s", cwd, arg);
      node = vfs_resolve_path(vfs_root, rel_path);
    }

    if (node && (node->flags & VFS_FILE)) {
      uint8_t buf[1024];
      uint32_t total_read = 0;
      while (total_read < node->length) {
        uint32_t to_read = (node->length - total_read > 1023)
                               ? 1023
                               : node->length - total_read;
        uint32_t read = vfs_read(node, total_read, to_read, buf);
        buf[read] = '\0';
        klog_print_str((char *)buf);
        total_read += read;
        if (read == 0)
          break;
      }
      klog_putchar('\n');
    } else {
      klog_print_str("File not found.\n");
    }
  } else if (strcmp(cmd, "touch") == 0) {
    if (strlen(arg) == 0) {
      klog_print_str("Usage: touch <filename>\n");
      return;
    }
    vfs_node_t *parent = vfs_resolve_path(vfs_root, cwd);
    if (vfs_create(parent, arg, 0) == 0) {
      klog_print_str("Created ");
      klog_print_str(arg);
      klog_putchar('\n');
    } else {
      klog_print_str("Failed to create file.\n");
    }
  } else if (strcmp(cmd, "rm") == 0) {
    if (strlen(arg) == 0) {
      klog_print_str("Usage: rm <filename>\n");
      return;
    }
    vfs_node_t *parent = vfs_resolve_path(vfs_root, cwd);
    if (vfs_remove(parent, arg) == 0) {
      klog_print_str("Removed ");
      klog_print_str(arg);
      klog_putchar('\n');
    } else {
      klog_print_str("Failed to remove file.\n");
    }
  } else if (strcmp(cmd, "mkdir") == 0) {
    if (strlen(arg) == 0) {
      klog_print_str("Usage: mkdir <dirname>\n");
      return;
    }
    vfs_node_t *parent = vfs_resolve_path(vfs_root, cwd);
    if (vfs_mkdir(parent, arg, 0) == 0) {
      klog_print_str("Created ");
      klog_print_str(arg);
      klog_putchar('\n');
    } else {
      klog_print_str("Failed to create directory.\n");
    }
  } else if (strcmp(cmd, "edit") == 0) {
    if (strlen(arg) == 0) {
      klog_print_str("Usage: edit <filename>\n");
      return;
    }

    // --- Start of new path resolution and editor logic ---
    vfs_node_t *file = NULL;
    char full_path[256];
    if (arg[0] == '/') {
        strncpy(full_path, arg, 256);
    } else {
        if (strcmp(cwd, "/") == 0)
            ksprintf(full_path, "/%s", arg);
        else
            ksprintf(full_path, "%s/%s", cwd, arg);
    }

    file = vfs_resolve_path(vfs_root, full_path);

    // If file doesn't exist, create it
    if (!file) {
        char path_buf[256];
        strncpy(path_buf, arg, 255);
        path_buf[255] = '\0';

        char *filename = path_buf;
        vfs_node_t *parent = NULL;

        int last_slash = -1;
        for (int i = 0; path_buf[i]; ++i) {
            if (path_buf[i] == '/') {
                last_slash = i;
            }
        }

        if (last_slash != -1) {
            filename = &path_buf[last_slash + 1];
            // Handle root case separately, e.g. /file.txt
            if (last_slash == 0) {
                parent = vfs_root;
            } else {
                path_buf[last_slash] = '\0'; // Cut string at slash
                char full_parent_path[256];
                if (path_buf[0] == '/') {
                    strncpy(full_parent_path, path_buf, 256);
                } else {
                    if (strcmp(cwd, "/") == 0)
                        ksprintf(full_parent_path, "/%s", path_buf);
                    else
                        ksprintf(full_parent_path, "%s/%s", cwd, path_buf);
                }
                parent = vfs_resolve_path(vfs_root, full_parent_path);
            }
        } else {
            // No slash, file is in current directory
            parent = vfs_resolve_path(vfs_root, cwd);
        }

        if (!parent || !(parent->flags & VFS_DIRECTORY)) {
            klog_print_str("Error: Parent directory not found or is not a directory.\n");
            return;
        }

        if (vfs_create(parent, filename, 0) == 0) {
            file = vfs_finddir(parent, filename);
        } else {
            klog_print_str("Error: Could not create file.\n");
            return;
        }
    }

    if (!file || !(file->flags & VFS_FILE)) {
      klog_print_str("Cannot edit this file.\n");
      return;
    }

    console_clear();
    extern uint32_t console_x, console_y;
    console_x = 0;
    console_y = 0;
    
    klog_print_str("--- Kyro-Editor --- Ctrl+S to Save | Ctrl+X to Exit ---\n");

    char *edit_buffer = (char *)kmalloc(4096);
    memset(edit_buffer, 0, 4096);
    int edit_idx = 0;
    
    // Load existing content
    if (file->length > 0) {
      edit_idx = vfs_read(file, 0, (file->length < 4095) ? file->length : 4095, (uint8_t*)edit_buffer);
      edit_buffer[edit_idx] = '\0';
      klog_print_str(edit_buffer);
    }

    // Event-based input loop
    while (1) {
      event_t ev;
      if (event_pop(&ev)) {
        if (ev.type == EVENT_KEY_DOWN) {
          char c = (char)ev.data1;
          uint8_t scancode = (uint8_t)ev.data2;
          
          bool ctrl = keyboard_is_ctrl_pressed();

          if (ctrl && (scancode == 0x2D)) { // Ctrl+X
            break;
          } else if (ctrl && (scancode == 0x1F)) { // Ctrl+S
            edit_buffer[edit_idx] = '\0';
            vfs_write(file, 0, edit_idx, (uint8_t *)edit_buffer);
            // Maybe add a small "(saved)" message on a status line in future
            continue;
          }

          if (c == '\n') {
            if (edit_idx < 4095) {
              edit_buffer[edit_idx++] = '\n';
              klog_putchar('\n');
            }
          } else if (c == '\b') {
            if (edit_idx > 0) {
              edit_idx--;
              klog_print_str("\b \b");
            }
          } else if (c >= 32 && c <= 126) {
            if (edit_idx < 4095) {
              edit_buffer[edit_idx++] = c;
              klog_putchar(c);
            }
          }
        }
      }
      fb_flush(); // Update screen with typed characters
      __asm__ __volatile__("hlt");
    }

    kfree(edit_buffer);
    console_clear();
    console_x = 0;
    console_y = 0;
    
    klog_print_str("\nExited editor.\n");
    // --- End of new logic ---
  } else if (strcmp(cmd, "kpm") == 0) {
    // Delegate to userspace kpm executable
    char kpm_path[128];
    ksprintf(kpm_path, "/bin/%s", cmd);

    // For now, we'll just execute kpm without arguments and it will likely print its own usage.
    // A proper argument passing mechanism for userspace executables would be needed to pass 'list', 'install', etc.
    klog(LOG_INFO, "SHELL: Executing userspace kpm.");
    if (elf_exec_as_thread(kpm_path) != 0) {
        klog(LOG_ERROR, "SHELL: Failed to execute userspace kpm.");
    }
  } else if (strcmp(cmd, "testpanic") == 0) {
    panic("User-triggered panic.", NULL);
  } else if (strcmp(cmd, "panic") == 0) { // Debug command for triggering kernel panic
    if (epstein == 1) {
        if (strlen(arg) > 0) {
            klog(LOG_ERROR, "SHELL: Debug panic triggered by user: %s", arg);
            struct registers dummy_regs;
            memset(&dummy_regs, 0, sizeof(struct registers));
            panic(arg, &dummy_regs); // Trigger kernel panic with user-provided reason and dummy registers
        } else {
            klog(LOG_WARN, "SHELL: Usage: panic <reason>");
        }
    } else {
        klog(LOG_INFO, "SHELL: Debug panic command disabled. Set epstein=1 to enable.");
    }
  } else if (strcmp(cmd, "clear") == 0) {
    fb_clear(0x00000000);
    extern uint32_t console_x, console_y;
    console_x = 0;
    console_y = 0;
  } else if (strcmp(cmd, "version") == 0) {
    klog_print_str(KYROOS_VERSION_STRING);
    klog_print_str(" (Build ");
    klog_print_str(KYROOS_VERSION_BUILD);
    klog_print_str(")\n");
  } else if (strcmp(cmd, "kyrofetch") == 0) {
    shell_kyrofetch();
  } else if (strcmp(cmd, "info") == 0) {
    char buf[256];
    klog_print_str("\n=== KyroOS System Information ===\n");
    ksprintf(buf, "Kernel:  %s (Build %s)\n", KYROOS_VERSION_STRING,
             KYROOS_VERSION_BUILD);
    klog_print_str(buf);
    klog_print_str("Arch:    x86_64 (64-bit long mode)\n");

    uint64_t total_mem = pmm_get_total_memory();
    uint64_t used_mem = pmm_get_used_memory();
    uint64_t free_mem = total_mem - used_mem;
    ksprintf(buf, "Memory:  %llu MB total, %llu MB used, %llu MB free\n",
             total_mem / 1024 / 1024, used_mem / 1024 / 1024,
             free_mem / 1024 / 1024);
    klog_print_str(buf);

    const fb_info_t *fb = fb_get_info();
    ksprintf(buf, "Display: %dx%d @ %dbpp\n", (int)fb->width, (int)fb->height,
             (int)fb->bpp);
    klog_print_str(buf);

    uint64_t ticks = timer_get_ticks();
    uint32_t uptime_sec = (uint32_t)(ticks / 100);
    ksprintf(buf, "Uptime:  %d seconds\n", uptime_sec);
    klog_print_str(buf);

    klog_print_str("Status:  IDT/GDT stable, VFS active, Scheduler running\n");
    klog_print_str("Network: E1000 driver loaded\n\n");
  } else if (strcmp(cmd, "reboot") == 0) {
    klog_print_str("Rebooting system...\n");
    // Wait for the keyboard controller input buffer to be empty
    uint8_t temp;
    while(inb(0x64) & 0x02) {
        temp = inb(0x60); // Read from the data port to clear it
    }
    // Send the reboot command to the keyboard controller
    outb(0x64, 0xFE);
    // The system should reboot. If not, halt.
    for(;;) { __asm__ __volatile__("cli; hlt"); }
  } else {
    // Try /bin/ for external commands
    char path[128];
    ksprintf(path, "/bin/%s", cmd);
    
    // Check if the command exists in /bin/
    vfs_node_t *node = vfs_resolve_path(vfs_root, path);
    if (node && (node->flags & VFS_FILE)) { // Ensure it's a file
      klog(LOG_INFO, "SHELL: Executing external command: %s", path);
      if (elf_exec_as_thread(path) != 0) {
          klog(LOG_ERROR, "SHELL: Failed to execute %s", path);
      }
      // After exec, the shell thread will likely yield or terminate,
      // and the scheduler will pick up the new userspace thread.
      // The shell should wait for the child process to finish before printing a new prompt.
      // For now, we just let the new thread run. A proper `waitpid` equivalent would be needed.
    } else {
      klog(LOG_INFO, "Unknown command: %s", cmd);
    }
  }
}

static void clear_current_line() {
  while (buffer_index > 0) {
    klog_print_str("\b \b");
    buffer_index--;
  }
}

void shell_main(void *arg) {
  klog(LOG_DEBUG, "shell_main: entered");
  (void)arg;
  shell_init();

  while (1) {
    event_t ev;
    if (event_pop(&ev)) {
      if (ev.type == EVENT_KEY_DOWN) {
        char c = (char)ev.data1;
        uint8_t scancode = (uint8_t)ev.data2;

        if (scancode == 0x48) { // Up Arrow
          if (history_count > 0) {
            if (current_history_view == -1)
              current_history_view = history_index;
            else if (current_history_view > 0)
              current_history_view--;
            else if (current_history_view == 0 && history_count == HISTORY_SIZE)
              current_history_view = HISTORY_SIZE - 1;

            clear_current_line();
            strncpy(line_buffer, history[current_history_view], BUFFER_SIZE);
            buffer_index = strlen(line_buffer);
            klog_print_str(line_buffer);
          }
        } else if (scancode == 0x50) { // Down Arrow
          if (current_history_view != -1) {
            current_history_view = (current_history_view + 1) % history_count;
            clear_current_line();
            strncpy(line_buffer, history[current_history_view], BUFFER_SIZE);
            buffer_index = strlen(line_buffer);
            klog_print_str(line_buffer);
          }
        } else if (c == '\n') {
          klog_putchar('\n');
          line_buffer[buffer_index] = '\0';
          execute_command(line_buffer);
          buffer_index = 0;
          klog_print_str("user@kyroos:");
          klog_print_str(cwd);
          klog_print_str("> ");
          current_history_view = -1;
        } else if (c == '\b') {
          if (buffer_index > 0) {
            buffer_index--;
            klog_print_str("\b \b");
          }
        } else if (c >= 32 && c <= 126) {
          if (buffer_index < BUFFER_SIZE - 1) {
            line_buffer[buffer_index++] = c;
            klog_putchar(c);
          }
        }
      }
    }
    fb_flush(); // Update the screen with all changes made in this loop iteration
    __asm__ __volatile__("hlt");
  }
}
