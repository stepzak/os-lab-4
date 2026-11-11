#include "app/spsc.h"

void spsc_init(spsc_queue_t *queue) {
    atomic_init(&queue->read_index, 0);
    atomic_init(&queue->write_index, 0);
}

message_t *spsc_acquire_slot(spsc_queue_t *queue) {
    unsigned write = atomic_load_explicit(&queue->write_index, memory_order_relaxed);
    unsigned next = (write + 1U) % SPSC_SLOT_COUNT;
    if (next == atomic_load_explicit(&queue->read_index, memory_order_acquire)) {
        return NULL;
    }
    return &queue->slots[write];
}

void spsc_commit_slot(spsc_queue_t *queue) {
    unsigned write = atomic_load_explicit(&queue->write_index, memory_order_relaxed);
    atomic_store_explicit(&queue->write_index, (write + 1U) % SPSC_SLOT_COUNT,
                          memory_order_release);
}

bool spsc_try_push(spsc_queue_t *queue, const message_t *message) {
    message_t *slot = spsc_acquire_slot(queue);
    if (slot == NULL) return false;
    *slot = *message;
    spsc_commit_slot(queue);
    return true;
}

const message_t *spsc_peek_slot(const spsc_queue_t *queue) {
    unsigned read = atomic_load_explicit(&queue->read_index, memory_order_relaxed);
    if (read == atomic_load_explicit(&queue->write_index, memory_order_acquire)) {
        return NULL;
    }
    return &queue->slots[read];
}

void spsc_consume_slot(spsc_queue_t *queue) {
    unsigned read = atomic_load_explicit(&queue->read_index, memory_order_relaxed);
    atomic_store_explicit(&queue->read_index, (read + 1U) % SPSC_SLOT_COUNT,
                          memory_order_release);
}

bool spsc_try_pop(spsc_queue_t *queue, message_t *message) {
    const message_t *slot = spsc_peek_slot(queue);
    if (slot == NULL) return false;
    *message = *slot;
    spsc_consume_slot(queue);
    return true;
}
