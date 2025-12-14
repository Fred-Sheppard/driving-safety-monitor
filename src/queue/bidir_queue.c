#include "bidir_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "bidir_queue";

#define BIDIR_QUEUE_SIZE 10

// Ring buffer structure
static struct {
    bidir_message_t buffer[BIDIR_QUEUE_SIZE];
    int head;  // Next write position
    int tail;  // Next read position
    int count; // Current number of items
    SemaphoreHandle_t mutex;
} queue;

void bidir_queue_init(void)
{
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;
    queue.mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Bidirectional queue initialized (size: %d)", BIDIR_QUEUE_SIZE);
}

bool bidir_queue_push(const bidir_message_t *msg)
{
    if (xSemaphoreTake(queue.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    // If full, overwrite oldest by advancing tail
    if (queue.count >= BIDIR_QUEUE_SIZE) {
        queue.tail = (queue.tail + 1) % BIDIR_QUEUE_SIZE;
        queue.count--;
        ESP_LOGW(TAG, "Queue full, overwriting oldest");
    }

    queue.buffer[queue.head] = *msg;
    queue.head = (queue.head + 1) % BIDIR_QUEUE_SIZE;
    queue.count++;

    xSemaphoreGive(queue.mutex);
    return true;
}

bool bidir_queue_pop(bidir_direction_t direction, bidir_message_t *msg)
{
    if (xSemaphoreTake(queue.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    // Search for a message with matching direction
    for (int i = 0; i < queue.count; i++) {
        int idx = (queue.tail + i) % BIDIR_QUEUE_SIZE;
        if (queue.buffer[idx].direction == direction) {
            // Found a match - copy it out
            *msg = queue.buffer[idx];

            // Shift remaining items to fill the gap
            for (int j = i; j < queue.count - 1; j++) {
                int curr = (queue.tail + j) % BIDIR_QUEUE_SIZE;
                int next = (queue.tail + j + 1) % BIDIR_QUEUE_SIZE;
                queue.buffer[curr] = queue.buffer[next];
            }

            queue.count--;
            // Adjust head back one position
            queue.head = (queue.head - 1 + BIDIR_QUEUE_SIZE) % BIDIR_QUEUE_SIZE;

            xSemaphoreGive(queue.mutex);
            return true;
        }
    }

    xSemaphoreGive(queue.mutex);
    return false;
}

bool bidir_queue_has(bidir_direction_t direction)
{
    if (xSemaphoreTake(queue.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    for (int i = 0; i < queue.count; i++) {
        int idx = (queue.tail + i) % BIDIR_QUEUE_SIZE;
        if (queue.buffer[idx].direction == direction) {
            xSemaphoreGive(queue.mutex);
            return true;
        }
    }

    xSemaphoreGive(queue.mutex);
    return false;
}

int bidir_queue_count(bidir_direction_t direction)
{
    if (xSemaphoreTake(queue.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < queue.count; i++) {
        int idx = (queue.tail + i) % BIDIR_QUEUE_SIZE;
        if (queue.buffer[idx].direction == direction) {
            count++;
        }
    }

    xSemaphoreGive(queue.mutex);
    return count;
}
