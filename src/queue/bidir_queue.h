#ifndef BIDIR_QUEUE_H
#define BIDIR_QUEUE_H

#include "message_types.h"
#include <stdbool.h>

/**
 * Bidirectional Command Queue
 *
 * A ring buffer that handles both inbound commands and outbound responses.
 * - MQTT task pushes inbound commands, pops outbound responses
 * - Processing task pops inbound commands, pushes outbound responses
 */

// Initialize the bidirectional queue
void bidir_queue_init(void);

// Push a message to the queue (returns false if full)
bool bidir_queue_push(const bidir_message_t *msg);

// Pop a message with specific direction (returns false if none available)
bool bidir_queue_pop(bidir_direction_t direction, bidir_message_t *msg);

// Check if queue has messages of a specific direction
bool bidir_queue_has(bidir_direction_t direction);

// Get count of messages by direction
int bidir_queue_count(bidir_direction_t direction);

#endif // BIDIR_QUEUE_H
