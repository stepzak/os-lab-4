#ifndef APP_CHANNEL_H
#define APP_CHANNEL_H

#include "app/spsc.h"

#define PEER_POLL_SPINS 1024U

typedef struct {
    spsc_queue_t requests;
    spsc_queue_t responses;
    atomic_uint cancelled;
    atomic_uint child_ready;
} channel_t;

static void channel_init(channel_t *channel) {
    spsc_init(&channel->requests);
    spsc_init(&channel->responses);
    atomic_init(&channel->cancelled, 0);
    atomic_init(&channel->child_ready, 0);
}

#endif
