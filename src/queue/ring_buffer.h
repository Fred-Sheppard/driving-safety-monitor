#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct ring_buffer ring_buffer_t;

// Returns NULL on allocation failure
ring_buffer_t *ring_buffer_create(size_t capacity, size_t item_size);

void ring_buffer_destroy(ring_buffer_t *rb);

// Overwrites oldest if full. If was_full is not NULL, sets to true if overwritten.
bool ring_buffer_push_back(ring_buffer_t *rb, const void *item, bool *was_full);

bool ring_buffer_push_front(ring_buffer_t *rb, const void *item, bool *was_full);

bool ring_buffer_pop_front(ring_buffer_t *rb, void *item);

bool ring_buffer_pop_back(ring_buffer_t *rb, void *item);

bool ring_buffer_peek(ring_buffer_t *rb, void *item);
size_t ring_buffer_count(ring_buffer_t *rb);
bool ring_buffer_is_empty(ring_buffer_t *rb);
bool ring_buffer_is_full(ring_buffer_t *rb);
size_t ring_buffer_capacity(ring_buffer_t *rb);
void ring_buffer_clear(ring_buffer_t *rb);

#endif // RING_BUFFER_H
