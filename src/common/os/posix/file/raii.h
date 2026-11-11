#ifndef FILE_RAII_H
#define FILE_RAII_H

#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>

#include "common/os/file.h"
#include "common/unlikely.h"
#include "common/raii.h"


static void os_file_cleanup(os_file_handle_t *fd) {
    (void)os_file_close(*fd);
}

static bool os_file_valid(const os_file_handle_t *fd) {
    return fd && *fd != OS_INVALID_HANDLE;
}

static void os_file_reset(os_file_handle_t *fd) {
    if (unlikely(fd == NULL || *fd == OS_INVALID_HANDLE)) return;
    *fd = OS_INVALID_HANDLE;
}

DEFINE_RAII_TYPE(os_file_handle, os_file_handle_t, os_file_cleanup, os_file_valid, os_file_reset)
DEFINE_RAII_TRIVIAL_MOVE(os_file_handle, os_file_valid, os_file_reset)

#endif //FILE_RAII_H
