#ifndef FILE_H
#define FILE_H

#include "common/os/common.h"

typedef enum {
    OS_FILE_READ = 1,
    OS_FILE_WRITE = 1 << 1,
    OS_FILE_CREATE = 1 << 2,
    OS_FILE_TRUNCATE = 1 << 3,
} os_file_flags_t;

os_file_handle_t os_file_open(const char *path, os_file_flags_t flags);
void os_file_close(os_file_handle_t file);

os_ssize_t os_file_read(os_file_handle_t file, void *buf, size_t count);
os_ssize_t os_file_read_full(os_file_handle_t file, void *buf, size_t count);

os_ssize_t os_file_write(os_file_handle_t file, const void *buf, size_t count);
os_ssize_t os_file_write_full(os_file_handle_t file, const void *buf, size_t count);
int os_file_fsync(os_file_handle_t file);

#endif //FILE_H
