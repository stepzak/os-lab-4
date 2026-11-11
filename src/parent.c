#include "app/channel.h"
#include "common/os/os.h"
#include "common/os/posix/file/raii.h"
#include "common/os/posix/mapped_shm/mapped_shm_raii.h"
#include "common/spin.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    channel_t *channel;
    os_process_handle_t process;
} child_process_t;

static void child_process_cleanup(child_process_t *child) {
    int saved_errno = errno;
    atomic_store_explicit(&child->channel->cancelled, 1, memory_order_release);
    if (child->process != OS_INVALID_HANDLE) {
        int exit_code;
        os_process_terminate(child->process);
        (void)os_process_wait(child->process, &exit_code);
    }
    errno = saved_errno;
}

static bool child_process_valid(const child_process_t *child) {
    return child != NULL && child->channel != NULL;
}

static void child_process_reset(child_process_t *child) {
    *child = (child_process_t){.process = OS_INVALID_HANDLE};
}

DEFINE_RAII_TYPE(child_process, child_process_t, child_process_cleanup,
                 child_process_valid, child_process_reset)

static volatile sig_atomic_t interrupted;

static void on_signal(int signum) {
    (void)signum;
    interrupted = 1;
}

static int install_signals(void) {
    struct sigaction action = {0};
    sigemptyset(&action.sa_mask);
    action.sa_handler = on_signal;
    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) return -1;
    action.sa_handler = SIG_IGN;
    return sigaction(SIGPIPE, &action, NULL);
}

static char *read_path(const char *prompt) {
    char *line = NULL;
    size_t capacity = 0;
    printf("%s", prompt);
    fflush(stdout);
    if (getline(&line, &capacity, stdin) < 0) {
        free(line);
        return NULL;
    }
    line[strcspn(line, "\r\n")] = '\0';
    return line;
}

static os_mapped_shm_t create_channel(void) {
    os_shm_info_t info = {
        .flags = OS_O_RDWR | OS_O_CREAT | OS_O_EXCL,
        .mode = OS_S_IRUSR | OS_S_IWUSR,
    };
    for (unsigned attempt = 0; attempt < 1024; ++attempt) {
        snprintf(info.name, sizeof(info.name), "/os_lab_4_%jd_%u",
                 (intmax_t)os_process_id(), attempt);
        os_mapped_shm_t shm = os_mapped_shm_create(
            &info, sizeof(channel_t), OS_PROT_READ | OS_PROT_WRITE, OS_MAP_SHARED);
        if (shm.mmap_addr != NULL || errno != EEXIST) return shm;
    }
    errno = EEXIST;
    return (os_mapped_shm_t){.fd = OS_INVALID_HANDLE};
}

static bool handle_response(const message_t *message, bool *done) {
    switch (message->kind) {
    case MESSAGE_RESULT:
        if (printf("Sum of %d numbers: %d\n", message->payload.result.count,
                   message->payload.result.sum) < 0) {
            perror("Failed to write result");
            return false;
        }
        return true;
    case MESSAGE_OVERFLOW:
        fprintf(stderr, "Integer overflow in line %zu while parsing or summing\n",
                message->payload.result.line);
        return false;
    case MESSAGE_DONE:
        *done = true;
        return true;
    default:
        fprintf(stderr, "Unexpected child message: %d\n", (int)message->kind);
        return false;
    }
}

static int run(const char *filename, const char *child_path) {
    raii(os_file_handle) file = os_file_open(filename, OS_FILE_READ);
    if (file == OS_INVALID_HANDLE) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }
    raii(os_mapped_shm) shm = create_channel();
    if (shm.mmap_addr == NULL) {
        perror("Failed to create shared memory");
        return EXIT_FAILURE;
    }
    channel_t *channel = shm.mmap_addr;
    channel_init(channel);

    char parent_pid[32];
    snprintf(parent_pid, sizeof(parent_pid), "%jd", (intmax_t)os_process_id());
    char *child_argv[] = {(char *)child_path, shm.name, parent_pid, NULL};
    raii(child_process) child = {
        .channel = channel,
        .process = os_process_spawn(child_path, child_argv),
    };
    if (child.process == OS_INVALID_HANDLE) {
        perror("Failed to start child");
        return EXIT_FAILURE;
    }

    int child_exit = EXIT_FAILURE;
    bool eof_sent = false;
    bool done = false;
    unsigned idle_spins = 0;
    while (!done) {
        if (interrupted) {
            fprintf(stderr, "Interrupted\n");
            return EXIT_FAILURE;
        }
        if (shm.is_owner && atomic_load_explicit(&channel->child_ready,
                                                 memory_order_acquire)) {
            if (os_shm_unlink(shm.name) < 0) {
                perror("Failed to unlink shared memory");
                return EXIT_FAILURE;
            }
            shm.is_owner = false;
        }
        bool progress = false;
        message_t response;
        for (unsigned i = 0; i < SPSC_SLOT_COUNT; ++i) {
            if (!spsc_try_pop(&channel->responses, &response)) break;
            progress = true;
            if (!handle_response(&response, &done)) return EXIT_FAILURE;
            if (done) break;
        }
        if (done) break;
        if (child.process == OS_INVALID_HANDLE) {
            fprintf(stderr, "Child exited without completion (status %d)\n", child_exit);
            return EXIT_FAILURE;
        }

        if (!eof_sent) {
            message_t *slot = spsc_acquire_slot(&channel->requests);
            if (slot != NULL) {
                os_ssize_t bytes = os_file_read(file, slot->payload.data.bytes,
                                               INPUT_BLOCK_SIZE);
                if (bytes < 0) {
                    perror("Failed to read input file");
                    return EXIT_FAILURE;
                }
                slot->kind = bytes == 0 ? MESSAGE_END : MESSAGE_DATA;
                slot->payload.data.size = (size_t)bytes;
                spsc_commit_slot(&channel->requests);
                eof_sent = bytes == 0;
                progress = true;
            }
        }
        if (!progress) {
            cpu_relax();
            if (++idle_spins < PEER_POLL_SPINS) continue;
            idle_spins = 0;
            int status = os_process_poll(child.process, &child_exit);
            if (status < 0) {
                perror("Failed to poll child");
                return EXIT_FAILURE;
            }
            if (status > 0) {
                child.process = OS_INVALID_HANDLE;
            }
        } else {
            idle_spins = 0;
        }
    }

    if (!eof_sent) {
        fprintf(stderr, "Child completed before receiving end of input\n");
        return EXIT_FAILURE;
    }
    if (child.process != OS_INVALID_HANDLE) {
        if (os_process_wait(child.process, &child_exit) < 0) {
            perror("Failed to wait for child");
            return EXIT_FAILURE;
        }
        child.process = OS_INVALID_HANDLE;
    }
    if (child_exit != EXIT_SUCCESS) {
        fprintf(stderr, "Child failed (status %d)\n", child_exit);
        return EXIT_FAILURE;
    }
    if ( fflush(stdout) == EOF) {
        perror("Failed to write to standard output");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 3) {
        fprintf(stderr, "Usage: %s [data_file child_executable]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (install_signals() < 0) {
        perror("Failed to install signal handlers");
        return EXIT_FAILURE;
    }
    if (argc == 3) return run(argv[1], argv[2]);

    char *filename = read_path("Enter filename: ");
    char *child_path = filename != NULL ? read_path("Enter path to the child process: ") : NULL;
    int result = EXIT_FAILURE;
    if (filename == NULL || child_path == NULL) {
        fprintf(stderr, "Failed to read paths from stdin\n");
    }
    else {
        result = run(filename, child_path);
    }
    free(filename);
    free(child_path);
    return result;
}
