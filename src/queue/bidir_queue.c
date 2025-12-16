#include "bidir_queue.h"
#include "ring_buffer.h"
#include "esp_log.h"

static const char *TAG = "bidir_queue";

#define BIDIR_QUEUE_SIZE 10

static ring_buffer_t *queue = NULL;

// Match function for direction-based filtering
static bool match_direction(const void *item, void *ctx)
{
    const bidir_message_t *msg = item;
    bidir_direction_t direction = *(bidir_direction_t *)ctx;
    return msg->direction == direction;
}

void bidir_queue_init(void)
{
    queue = ring_buffer_create(BIDIR_QUEUE_SIZE, sizeof(bidir_message_t));
    if (queue) {
        ESP_LOGI(TAG, "Bidirectional queue initialized (size: %d)", BIDIR_QUEUE_SIZE);
    } else {
        ESP_LOGE(TAG, "Failed to initialize bidirectional queue");
    }
}

bool bidir_queue_push(const bidir_message_t *msg)
{
    if (!queue || !msg) return false;
    bool was_full = false;
    bool ok = ring_buffer_push(queue, msg, &was_full);
    if (ok && was_full)
        ESP_LOGW(TAG, "bidir_queue full, overwrote oldest message");
    return ok;
}

bool bidir_queue_pop(bidir_direction_t direction, bidir_message_t *msg)
{
    if (!queue || !msg) return false;
    return ring_buffer_pop_match(queue, match_direction, &direction, msg);
}

bool bidir_queue_has(bidir_direction_t direction)
{
    if (!queue) return false;
    return ring_buffer_has_match(queue, match_direction, &direction);
}

int bidir_queue_count(bidir_direction_t direction)
{
    if (!queue) return 0;
    return (int)ring_buffer_count_match(queue, match_direction, &direction);
}
