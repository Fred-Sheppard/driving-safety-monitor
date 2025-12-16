#include "policy_based_queue.h"
#include "config.h"
#include "esp_log.h"

static const char *TAG = "policy_based_queue";

#if QUEUE_OVERFLOW_POLICY == QUEUE_POLICY_KEEP_OLDEST
static bool queue_send_keep_oldest(QueueHandle_t queue, const void *item)
{
    bool sent = xQueueSend(queue, item, 0) == pdTRUE;
    if (!sent)
    {
        ESP_LOGW(TAG, "Queue full. Item not queued");
    };
    return sent;
}
#else

static bool queue_send_keep_newest(QueueHandle_t queue, const void *item, size_t item_size)
{
    if (xQueueSend(queue, item, 0) == pdTRUE)
    {
        return true;
    }

    // Queue full - remove oldest to make room (heap alloc to avoid stack overflow)
    void *discard = pvPortMalloc(item_size);
    if (discard != NULL)
    {
        if (xQueueReceive(queue, discard, 0) == pdTRUE)
        {
            ESP_LOGW(TAG, "Queue full. Dropping oldest item");
        }
        vPortFree(discard);
    }

    if (xQueueSend(queue, item, 0) == pdFALSE)
    {
        ESP_LOGW(TAG, "Race condition: Unable to push to queue after dropping oldest item.");
    };
    // Indicate that an item was dropped
    return false;
}
#endif

bool send_to_queue(QueueHandle_t queue, const void *item, size_t item_size)
{
#if QUEUE_OVERFLOW_POLICY == QUEUE_POLICY_KEEP_NEWEST
    return queue_send_keep_newest(queue, item, item_size);
#else
    (void)item_size;
    return queue_send_keep_oldest(queue, item);
#endif
}
