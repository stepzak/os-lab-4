#ifndef SHM_H
#define SHM_H

#include <stdbool.h>
#include <stddef.h>

#include "common/os/common.h"

#define SHM_NAME_LEN 127

typedef os_file_handle_t os_shm_file_t;

typedef enum {
    OS_O_RDONLY = 1,
    OS_O_RDWR = 1 << 2,
    OS_O_CREAT = 1 << 3,
    OS_O_TRUNC = 1 << 4,
    OS_O_EXCL = 1 << 5,
} os_o_flags_t;

typedef enum {
    OS_S_IRUSR = 00400,
    OS_S_IWUSR = 00200,
    OS_S_IXUSR = 00100,
    OS_S_IRGRP = 00040,
    OS_S_IWGRP = 00020,
    OS_S_IXGRP = 00010,
    OS_S_IROTH = 00004,
    OS_S_IWOTH = 00002,
    OS_S_IXOTH = 00001,
} os_mode_t;

os_shm_file_t os_shm_open(const char *name, os_o_flags_t flags, os_mode_t mode);
int os_shm_unlink(const char *name);

#endif //SHM_H
