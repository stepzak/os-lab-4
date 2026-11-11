#include "common/os/shm.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

static mode_t posix_mode(os_mode_t mode) {
    mode_t result = 0;
    if (mode & OS_S_IRUSR) result |= S_IRUSR;
    if (mode & OS_S_IWUSR) result |= S_IWUSR;
    if (mode & OS_S_IXUSR) result |= S_IXUSR;
    if (mode & OS_S_IRGRP) result |= S_IRGRP;
    if (mode & OS_S_IWGRP) result |= S_IWGRP;
    if (mode & OS_S_IXGRP) result |= S_IXGRP;
    if (mode & OS_S_IROTH) result |= S_IROTH;
    if (mode & OS_S_IWOTH) result |= S_IWOTH;
    if (mode & OS_S_IXOTH) result |= S_IXOTH;

    return result;
}

os_shm_file_t os_shm_open(const char *name, os_o_flags_t flags, os_mode_t mode) {
    int oflag = 0;
    if (flags & OS_O_RDWR) {
        oflag |= O_RDWR;
    } else if (flags & OS_O_RDONLY) {
        oflag |= O_RDONLY;
    }

    if (flags & OS_O_CREAT) {
        oflag |= O_CREAT;
    }
    if (flags & OS_O_EXCL) oflag |= O_EXCL;
    if (flags & OS_O_TRUNC) {
        oflag |= O_TRUNC;
    }
    mode_t pmode = posix_mode(mode);
    os_shm_file_t fd = (os_shm_file_t)shm_open(name, oflag, pmode);
    return fd;
}

int os_shm_unlink(const char *name) {
    return shm_unlink(name);
}
