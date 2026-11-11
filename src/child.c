#include "app/channel.h"
#include "app/parser.h"
#include "common/os/os.h"
#include "common/os/posix/mapped_shm/mapped_shm_raii.h"
#include "common/spin.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    os_process_handle_t parent;
    unsigned idle_spins;
} parent_watch_t;

static bool parent_disappeared(parent_watch_t *watch) {
    if (++watch->idle_spins < PEER_POLL_SPINS) return false;
    watch->idle_spins = 0;
    return os_process_parent_id() != watch->parent;
}

static bool send_response(channel_t *channel, parent_watch_t *watch,
                          const message_t *message) {
    while (!atomic_load_explicit(&channel->cancelled, memory_order_acquire)) {
        if (spsc_try_push(&channel->responses, message)) return true;
        if (parent_disappeared(watch)) return false;
        cpu_relax();
    }
    return false;
}

static int process_input(channel_t *channel, os_process_handle_t parent) {
    parser_t parser;
    parser_init(&parser);
    parent_watch_t watch = {.parent = parent};
    while (!atomic_load_explicit(&channel->cancelled, memory_order_acquire)) {
        const message_t *request = spsc_peek_slot(&channel->requests);

        if (request == NULL) {
            if (parent_disappeared(&watch)) return EXIT_FAILURE;
            cpu_relax();
            continue;
        }
        message_t response;
        if (request->kind == MESSAGE_END) {
            spsc_consume_slot(&channel->requests);
            if (!parser_flush(&parser)) {
                message_t error = {
                    .kind = MESSAGE_OVERFLOW,
                    .payload.result = {.line = parser.line},
                };
                (void)send_response(channel, &watch, &error);
                return EXIT_FAILURE;
            }
            if (parser_take_line(&parser, &response) &&
                !send_response(channel, &watch, &response)) return EXIT_FAILURE;
            response = (message_t){.kind = MESSAGE_DONE};
            return send_response(channel, &watch, &response) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        if (request->kind != MESSAGE_DATA || request->payload.data.size > INPUT_BLOCK_SIZE) {
            fprintf(stderr, "Invalid parent message\n");
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < request->payload.data.size; ++i) {
            char ch = request->payload.data.bytes[i];
            if (!parser_feed(&parser, ch)) {
                message_t error = {
                    .kind = MESSAGE_OVERFLOW,
                    .payload.result = {.line = parser.line},
                };
                (void)send_response(channel, &watch, &error);
                return EXIT_FAILURE;
            }
            if (ch == '\n' && parser_take_line(&parser, &response) &&
                !send_response(channel, &watch, &response)) return EXIT_FAILURE;
        }
        spsc_consume_slot(&channel->requests);

    }
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strlen(argv[1]) >= SHM_NAME_LEN) {
        fprintf(stderr, "Usage: %s shm_name parent_pid (started by parent)\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *end;
    errno = 0;
    intmax_t pid = strtoimax(argv[2], &end, 10);
    if (errno != 0 || *end != '\0' || pid <= 0 || pid > INTPTR_MAX ||
        os_process_parent_id() != (os_process_handle_t)pid) {
        fprintf(stderr, "Invalid parent PID\n");
        return EXIT_FAILURE;
    }
    os_shm_info_t info = {.flags = OS_O_RDWR};
    memcpy(info.name, argv[1], strlen(argv[1]) + 1);
    raii(os_mapped_shm) shm = os_mapped_shm_create(
        &info, sizeof(channel_t), OS_PROT_READ | OS_PROT_WRITE, OS_MAP_SHARED);
    if (shm.mmap_addr == NULL) {
        perror("Failed to open shared memory");
        return EXIT_FAILURE;
    }
    channel_t *channel = shm.mmap_addr;
    atomic_store_explicit(&channel->child_ready, 1, memory_order_release);
    return process_input(channel, (os_process_handle_t)pid);
}
