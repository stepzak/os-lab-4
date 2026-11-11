#ifndef MAPPED_SHM_H
#define MAPPED_SHM_H

#include "common/os/mmap.h"
#include "common/os/shm.h"

typedef struct {
    char name[SHM_NAME_LEN];
    os_o_flags_t flags;
    os_mode_t mode;
} os_shm_info_t;

typedef struct {
    os_shm_file_t fd;
    size_t size;
    char name[SHM_NAME_LEN];
    bool is_owner;
    void *mmap_addr;
} os_mapped_shm_t;

os_mapped_shm_t os_mapped_shm_create(os_shm_info_t *shm_info, size_t len, os_map_prot_t prot, os_map_flags_t flags);
void os_mapped_shm_destroy(os_mapped_shm_t *shm);

#endif //MAPPED_SHM_H
