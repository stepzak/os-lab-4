#ifndef MMAP_H
#define MMAP_H

#include "common/os/common.h"

typedef enum {
    OS_PROT_EXEC = 1,
    OS_PROT_READ = 1 << 1,
    OS_PROT_WRITE = 1 << 2,
    OS_PROT_NONE = 1 << 3,
} os_map_prot_t;

typedef enum {
    OS_MAP_SHARED = 1,
    OS_MAP_PRIVATE = 1 << 1,
    OS_MAP_FIXED = 1 << 3,
} os_map_flags_t;

void *os_mmap(void *addr,
    size_t len,
    os_map_prot_t prot,
    os_map_flags_t flags,
    os_file_handle_t fd,
    os_offset_t offset);

int os_munmap(void *addr, size_t len);

#endif //MMAP_H
