#ifndef APP_SPSC_H
#define APP_SPSC_H

#include <stdbool.h>
#include <stdatomic.h>

#include "app/protocol.h"

#define SPSC_SLOT_COUNT 256U
#define SPSC_CACHE_LINE_SIZE 64U

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Always lock-free unsigned atomics required");
_Static_assert((SPSC_SLOT_COUNT & (SPSC_SLOT_COUNT - 1U)) == 0U && SPSC_SLOT_COUNT >= 2U,
               "SPSC_SLOT_COUNT must be a power of two >= 2");


typedef struct {
    _Alignas(SPSC_CACHE_LINE_SIZE) atomic_uint read_index;
    _Alignas(SPSC_CACHE_LINE_SIZE) atomic_uint write_index;
    _Alignas(SPSC_CACHE_LINE_SIZE) message_t slots[SPSC_SLOT_COUNT];
} spsc_queue_t;

void spsc_init(spsc_queue_t *queue);

message_t *spsc_acquire_slot(spsc_queue_t *queue);
void spsc_commit_slot(spsc_queue_t *queue);
bool spsc_try_push(spsc_queue_t *queue, const message_t *message);

bool spsc_try_pop(spsc_queue_t *queue, message_t *message);
const message_t *spsc_peek_slot(const spsc_queue_t *queue);
void spsc_consume_slot(spsc_queue_t *queue);

#endif
