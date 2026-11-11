#ifndef MAPPED_SHM_RAII_H
#define MAPPED_SHM_RAII_H

#include "common/os/mapped_shm.h"
#include "common/unlikely.h"
#include "common/raii.h"

static void mapped_shm_cleanup(os_mapped_shm_t *shm) {
    os_mapped_shm_destroy(shm);
}

static bool mapped_shm_valid(const os_mapped_shm_t *shm) {
    return shm != NULL && ((shm->mmap_addr != NULL && shm->size > 0) ||
                           (shm->is_owner && shm->name[0] != '\0'));
}

static void mapped_shm_reset(os_mapped_shm_t *shm) {
    if (unlikely(shm == NULL)) return;
    shm->fd = OS_INVALID_HANDLE;
    shm->name[0] = '\0';
    shm->mmap_addr = NULL;
    shm->size = 0;
    shm->is_owner = false;
}

DEFINE_RAII_TYPE(os_mapped_shm, os_mapped_shm_t, mapped_shm_cleanup, mapped_shm_valid, mapped_shm_reset)
DEFINE_RAII_TRIVIAL_MOVE(os_mapped_shm, mapped_shm_valid, mapped_shm_reset)

#endif //MAPPED_SHM_RAII_H
