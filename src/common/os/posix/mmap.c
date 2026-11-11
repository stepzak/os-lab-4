#include "common/os/mmap.h"

#include <sys/mman.h>

static int posix_prot(os_map_prot_t prot) {
    int res = 0;
    if (prot & OS_PROT_EXEC) res |= PROT_EXEC;
    if (prot & OS_PROT_WRITE) res |= PROT_WRITE;
    if (prot & OS_PROT_READ) res |= PROT_READ;
    if (prot & OS_PROT_NONE) res |= PROT_NONE;

    return res;
}

static int posix_flags(os_map_flags_t flags) {
    int res = 0;
    if (flags & OS_MAP_PRIVATE) res |= MAP_PRIVATE;
    if (flags & OS_MAP_SHARED) res |= MAP_SHARED;
    if (flags & OS_MAP_FIXED) res |= MAP_FIXED;
    return res;
}

void *os_mmap(void *addr,
    size_t len,
    os_map_prot_t prot,
    os_map_flags_t flags,
    os_file_handle_t fd,
    os_offset_t offset)
{
    int pprot = posix_prot(prot);
    int pflags = posix_flags(flags);
    return mmap(addr, len, pprot, pflags, (int)fd, offset);
}

int os_munmap(void *addr, size_t len) {
    return munmap(addr, len);
}
