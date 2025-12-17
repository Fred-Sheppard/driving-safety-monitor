#include "ring_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ring_buffer";

#define MUTEX_TIMEOUT_MS 100

struct ring_buffer
{
    uint8_t *buffer;
    size_t capacity;
    size_t item_size;
    size_t head;
    size_t tail;
    size_t count;
    SemaphoreHandle_t mutex;
};

ring_buffer_t *ring_buffer_create(size_t capacity, size_t item_size)
{
    ring_buffer_t *rb = pvPortMalloc(sizeof(ring_buffer_t));
    if (!rb)
    {
        ESP_LOGE(TAG, "Failed to allocate ring buffer struct");
        return NULL;
    }

    rb->buffer = pvPortMalloc(capacity * item_size);
    if (!rb->buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate ring buffer storage");
        vPortFree(rb);
        return NULL;
    }

    rb->mutex = xSemaphoreCreateMutex();
    if (!rb->mutex)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        vPortFree(rb->buffer);
        vPortFree(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->item_size = item_size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    ESP_LOGI(TAG, "Created ring buffer: capacity=%zu, item_size=%zu", capacity, item_size);
    return rb;
}

void ring_buffer_destroy(ring_buffer_t *rb)
{
    if (!rb)
        return;
    if (rb->mutex)
        vSemaphoreDelete(rb->mutex);
    vPortFree(rb->buffer);
    vPortFree(rb);
}

static inline void *item_ptr(ring_buffer_t *rb, size_t index)
{
    return rb->buffer + (index * rb->item_size);
}

bool ring_buffer_push(ring_buffer_t *rb, const void *item, bool *was_full)
{
    if (!rb || !item)
        return false;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }

    bool full = rb->count >= rb->capacity;
    if (full)
    {
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count--;
        ESP_LOGW(TAG, "Ring buffer full. Overwriting oldest element");
    }
    // NULL check
    if (was_full)
        *was_full = full;

    memcpy(item_ptr(rb, rb->head), item, rb->item_size);
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;

    xSemaphoreGive(rb->mutex);
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, void *item)
{
    if (!rb || !item)
        return false;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }

    if (rb->count == 0)
    {
        xSemaphoreGive(rb->mutex);
        return false;
    }

    memcpy(item, item_ptr(rb, rb->tail), rb->item_size);
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;

    xSemaphoreGive(rb->mutex);
    return true;
}

bool ring_buffer_peek(ring_buffer_t *rb, void *item)
{
    if (!rb || !item)
        return false;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }

    if (rb->count == 0)
    {
        xSemaphoreGive(rb->mutex);
        return false;
    }

    memcpy(item, item_ptr(rb, rb->tail), rb->item_size);
    xSemaphoreGive(rb->mutex);
    return true;
}

size_t ring_buffer_count(ring_buffer_t *rb)
{
    if (!rb)
        return 0;
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return 0;
    }
    size_t count = rb->count;
    xSemaphoreGive(rb->mutex);
    return count;
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    return ring_buffer_count(rb) == 0;
}

bool ring_buffer_is_full(ring_buffer_t *rb)
{
    if (!rb)
        return false;
    return ring_buffer_count(rb) >= rb->capacity;
}

size_t ring_buffer_capacity(ring_buffer_t *rb)
{
    return rb ? rb->capacity : 0;
}

void ring_buffer_clear(ring_buffer_t *rb)
{
    if (!rb)
        return;
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    xSemaphoreGive(rb->mutex);
}

bool ring_buffer_pop_match(ring_buffer_t *rb, ring_buffer_match_fn match,
                           void *ctx, void *item)
{
    if (!rb || !match || !item)
        return false;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }

    for (size_t i = 0; i < rb->count; i++)
    {
        size_t idx = (rb->tail + i) % rb->capacity;
        void *curr = item_ptr(rb, idx);

        if (match(curr, ctx))
        {
            memcpy(item, curr, rb->item_size);

            // Shift to fill gap
            for (size_t j = i; j < rb->count - 1; j++)
            {
                size_t curr_idx = (rb->tail + j) % rb->capacity;
                size_t next_idx = (rb->tail + j + 1) % rb->capacity;
                memcpy(item_ptr(rb, curr_idx), item_ptr(rb, next_idx), rb->item_size);
            }

            rb->count--;
            rb->head = (rb->head + rb->capacity - 1) % rb->capacity;
            xSemaphoreGive(rb->mutex);
            return true;
        }
    }

    xSemaphoreGive(rb->mutex);
    return false;
}

bool ring_buffer_has_match(ring_buffer_t *rb, ring_buffer_match_fn match, void *ctx)
{
    if (!rb || !match)
        return false;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }

    for (size_t i = 0; i < rb->count; i++)
    {
        size_t idx = (rb->tail + i) % rb->capacity;
        if (match(item_ptr(rb, idx), ctx))
        {
            xSemaphoreGive(rb->mutex);
            return true;
        }
    }

    xSemaphoreGive(rb->mutex);
    return false;
}

size_t ring_buffer_count_match(ring_buffer_t *rb, ring_buffer_match_fn match, void *ctx)
{
    if (!rb || !match)
        return 0;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < rb->count; i++)
    {
        size_t idx = (rb->tail + i) % rb->capacity;
        if (match(item_ptr(rb, idx), ctx))
        {
            count++;
        }
    }

    xSemaphoreGive(rb->mutex);
    return count;
}
