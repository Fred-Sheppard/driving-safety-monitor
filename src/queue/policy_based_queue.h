#ifndef policy_based_queue_H
#define policy_based_queue_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>

/**
 * Send item to queue with configurable overflow policy.
 * Policy is determined by QUEUE_OVERFLOW_POLICY in config.h:
 * - QUEUE_POLICY_KEEP_OLDEST: Drop new item if queue full
 * - QUEUE_POLICY_KEEP_NEWEST: Drop oldest item to make room for new
 *
 * @param queue The queue handle
 * @param item Pointer to item to send
 * @param item_size Size of the item (for KEEP_NEWEST policy)
 * @return true if item was queued, false if dropped
 */
bool send_to_queue(QueueHandle_t queue, const void *item, size_t item_size);

#endif
