#include <kyroolib.h>

#define BUF_SIZE 1024

#define BUF_SIZE 1024

// Helper function to recursively copy files and directories
static int copy_recursive(const char *src, const char *dest) {
    struct stat st;
    if (stat(src, &st) != 0) {
        print("cp: could not stat source '"); print(src); print("'\n");
        return -1;
    }

    if (S_ISREG(st.st_mode)) { // It's a regular file
        int src_fd = open(src, O_RDONLY);
        if (src_fd < 0) {
            print("cp: cannot open source file '"); print(src); print("'\n");
            return -1;
        }

        int dest_fd = open(dest, O_CREAT | O_TRUNC | O_WRONLY);
        if (dest_fd < 0) {
            print("cp: cannot create destination file '"); print(dest); print("'\n");
            close(src_fd);
            return -1;
        }

        char buffer[BUF_SIZE];
        int bytes_read;
        while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
            if (write(dest_fd, buffer, bytes_read) < 0) {
                print("cp: write error to '"); print(dest); print("'\n");
                close(src_fd);
                close(dest_fd);
                return -1;
            }
        }
        close(src_fd);
        close(dest_fd);
        return 0;
    } else if (S_ISDIR(st.st_mode)) { // It's a directory
        if (mkdir(dest) != 0) {
            // If it exists, that's fine. If it's another error, return it.
            // Our current mkdir might not return specific error codes for "already exists".
            // For now, if mkdir returns <0, we consider it an error unless we can positively check for "already exists"
            struct stat dest_st;
            if (stat(dest, &dest_st) != 0 || !S_ISDIR(dest_st.st_mode)) {
                print("cp: cannot create directory '"); print(dest); print("'\n");
                return -1;
            }
        }

        int dir_fd = open(src, O_RDONLY); // Open source directory for reading entries
        if (dir_fd < 0) {
            print("cp: cannot open source directory '"); print(src); print("'\n");
            return -1;
        }

        struct dirent de;
        int index = 0;
        char src_entry_path[MAX_FILENAME_LEN * 2];
        char dest_entry_path[MAX_FILENAME_LEN * 2];

        while (readdir(dir_fd, index++, &de)) {
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                continue;
            }

            ksprintf(src_entry_path, "%s/%s", src, de.name);
            ksprintf(dest_entry_path, "%s/%s", dest, de.name);
            
            if (copy_recursive(src_entry_path, dest_entry_path) != 0) {
                close(dir_fd);
                return -1; // Propagate error
            }
        }
        close(dir_fd);
        return 0;
    }
    print("cp: unsupported source type '"); print(src); print("'\n");
    return -1;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print("cp: usage: cp <source> <destination>\n");
        return 1;
    }

    const char *src_path = argv[1];
    const char *dest_path = argv[2];

    if (copy_recursive(src_path, dest_path) != 0) {
        return 1;
    }

    return 0;
}