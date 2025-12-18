#ifndef RING_BUFFER_UTILS_H
#define RING_BUFFER_UTILS_H

#include "esp_log.h"
#include "queue/ring_buffer.h"

// Helper that logs when the ring buffer is full
static inline bool ring_buffer_push_back_with_full_log_tag(const char *tag,
                                                           ring_buffer_t *rb,
                                                           const void *item,
                                                           const char *full_msg)
{
    bool was_full = false;
    bool ok = ring_buffer_push_back(rb, item, &was_full);

    if (ok && was_full)
    {
        ESP_LOGW(tag, "%s", full_msg);
    }

    return ok;
}

// Convenience macro that uses the local TAG symbol
#define ring_buffer_push_back_with_full_log(rb, item, full_msg) \
    ring_buffer_push_back_with_full_log_tag(TAG, (rb), (item), (full_msg))

#endif // RING_BUFFER_UTILS_H


