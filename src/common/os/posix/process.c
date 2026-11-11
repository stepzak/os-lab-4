#include "common/os/process.h"
#include "common/os/posix/file/raii.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

os_process_handle_t os_process_spawn(const char *path, char *const argv[]) {
    if (unlikely(path == NULL || argv == NULL)) { errno = EINVAL; return OS_INVALID_HANDLE; }
    int fds[2];
    if (pipe(fds) < 0) return OS_INVALID_HANDLE;
    raii(os_file_handle) parent_read = fds[0];
    raii(os_file_handle) child_write = fds[1];
    for (int i = 0; i < 2; ++i) {
        int flags = fcntl(fds[i], F_GETFD);
        if (flags < 0 || fcntl(fds[i], F_SETFD, flags | FD_CLOEXEC) < 0)
            return OS_INVALID_HANDLE;
    }

    pid_t pid = fork();
    if (pid < 0) return OS_INVALID_HANDLE;
    if (pid == 0) {
        close((int)move(os_file_handle, parent_read));
        execv(path, argv);
        int child_errno = errno;
        (void)os_file_write_full(child_write, &child_errno, sizeof(child_errno));
        _exit(EXIT_FAILURE);
    }

    close((int)move(os_file_handle, child_write));
    int child_errno = 0;
    os_ssize_t bytes = os_file_read_full(parent_read, &child_errno, sizeof(child_errno));
    if (bytes != 0) {
        int saved_errno = bytes < 0 ? errno :
            bytes == (os_ssize_t)sizeof(child_errno) ? child_errno : EIO;
        int exit_code;
        if (bytes != (os_ssize_t)sizeof(child_errno)) os_process_terminate(pid);
        (void)os_process_wait(pid, &exit_code);
        errno = saved_errno;
        return OS_INVALID_HANDLE;
    }
    return pid;
}

static int wait_process(os_process_handle_t process, int *exit_code, int flags) {
    int status;
    pid_t result;
    do {
        result = waitpid((pid_t)process, &status, flags);
    } while (result < 0 && errno == EINTR);
    if (result <= 0) return (int)result;
    *exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    return 1;
}

int os_process_poll(os_process_handle_t process, int *exit_code) {
    return wait_process(process, exit_code, WNOHANG);
}

int os_process_wait(os_process_handle_t process, int *exit_code) {
    return wait_process(process, exit_code, 0);
}

void os_process_terminate(os_process_handle_t process) {
    if (process > 0) (void)kill((pid_t)process, SIGTERM);
}

os_process_handle_t os_process_id(void) { return getpid(); }
os_process_handle_t os_process_parent_id(void) { return getppid(); }
