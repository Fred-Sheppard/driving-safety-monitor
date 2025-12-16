#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Generic Thread-Safe Ring Buffer
 *
 * A circular buffer that overwrites oldest data when full.
 * Uses FreeRTOS mutex for thread-safe access between tasks.
 *
 * Usage:
 *   ring_buffer_t *rb = ring_buffer_create(10, sizeof(my_type_t));
 *   ring_buffer_push(rb, &my_item);
 *   ring_buffer_pop(rb, &out_item);
 *   ring_buffer_destroy(rb);
 */

typedef struct ring_buffer ring_buffer_t;

// Create a ring buffer with given capacity and item size
// Returns NULL on allocation failure
ring_buffer_t *ring_buffer_create(size_t capacity, size_t item_size);

// Destroy the ring buffer and free all resources
void ring_buffer_destroy(ring_buffer_t *rb);

// Push an item to the buffer (overwrites oldest if full)
// Returns true on success, false if mutex timeout
bool ring_buffer_push(ring_buffer_t *rb, const void *item);

// Pop the oldest item from the buffer
// Returns true if item was popped, false if empty or mutex timeout
bool ring_buffer_pop(ring_buffer_t *rb, void *item);

// Peek at the oldest item without removing
// Returns true if item was copied, false if empty or mutex timeout
bool ring_buffer_peek(ring_buffer_t *rb, void *item);

// Get current number of items in buffer
size_t ring_buffer_count(ring_buffer_t *rb);

// Check if buffer is empty
bool ring_buffer_is_empty(ring_buffer_t *rb);

// Check if buffer is full
bool ring_buffer_is_full(ring_buffer_t *rb);

// Get buffer capacity
size_t ring_buffer_capacity(ring_buffer_t *rb);

// Clear all items from the buffer
void ring_buffer_clear(ring_buffer_t *rb);

// Match function type for filtered operations
// Returns true if item matches the criteria
typedef bool (*ring_buffer_match_fn)(const void *item, void *ctx);

// Pop the first item matching the predicate (removes from middle if needed)
// Returns true if matching item was found and popped
bool ring_buffer_pop_match(ring_buffer_t *rb, ring_buffer_match_fn match,
                           void *ctx, void *item);

// Check if any item matches the predicate
bool ring_buffer_has_match(ring_buffer_t *rb, ring_buffer_match_fn match, void *ctx);

// Count items matching the predicate
size_t ring_buffer_count_match(ring_buffer_t *rb, ring_buffer_match_fn match, void *ctx);

#endif // RING_BUFFER_H
