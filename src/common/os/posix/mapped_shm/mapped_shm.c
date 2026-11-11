#include "common/os/mapped_shm.h"
#include "common/os/posix/file/raii.h"
#include "common/os/posix/mapped_shm/mapped_shm_raii.h"

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

os_mapped_shm_t os_mapped_shm_create(os_shm_info_t *info, size_t len,
                                    os_map_prot_t prot, os_map_flags_t flags) {
    const os_mapped_shm_t invalid = {.fd = OS_INVALID_HANDLE};
    if (info == NULL || len == 0 || info->name[0] != '/' ||
        strnlen(info->name, SHM_NAME_LEN) >= SHM_NAME_LEN) {
        errno = EINVAL;
        return invalid;
    }

    /* Creation is exclusive: never truncate or unlink another owner's object. */
    bool owner = (info->flags & OS_O_CREAT) != 0;
    os_o_flags_t open_flags = info->flags;
    if (owner) open_flags |= OS_O_EXCL;
    else if (open_flags & OS_O_TRUNC) {
        errno = EINVAL;
        return invalid;
    }
    raii(os_file_handle) fd = os_shm_open(info->name, open_flags, info->mode);
    if (fd == OS_INVALID_HANDLE) return invalid;

    raii(os_mapped_shm) result = {
        .fd = OS_INVALID_HANDLE,
        .size = len,
        .is_owner = owner,
    };
    memcpy(result.name, info->name, strlen(info->name) + 1);
    if (owner) {
        if (ftruncate((int)fd, (off_t)len) < 0) return invalid;
    } else {
        struct stat st;
        if (fstat((int)fd, &st) < 0) return invalid;
        if (st.st_size < 0 || (uintmax_t)st.st_size != (uintmax_t)len) {
            errno = EINVAL;
            return invalid;
        }
    }
    void *addr = os_mmap(NULL, len, prot, flags, fd, 0);
    if (addr == MAP_FAILED) return invalid;
    result.mmap_addr = addr;
    return move(os_mapped_shm, result);
}

void os_mapped_shm_destroy(os_mapped_shm_t *shm) {
    if (shm == NULL) return;
    int saved_errno = errno;
    if (shm->mmap_addr != NULL && shm->size > 0)
        os_munmap(shm->mmap_addr, shm->size);
    if (shm->is_owner && shm->name[0] != '\0') os_shm_unlink(shm->name);
    *shm = (os_mapped_shm_t){.fd = OS_INVALID_HANDLE};
    errno = saved_errno;
}
