#include "trace.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "trace";

// Buffer for vTaskGetRunTimeStats
#define STATS_BUFFER_SIZE 1024
static char stats_buffer[STATS_BUFFER_SIZE];

// Track previous task for switch logging
static const char *previous_task_name = NULL;

// Stats printing interval (ms)
#define STATS_INTERVAL_MS 5000

esp_err_t trace_init(void)
{
    ESP_LOGI(TAG, "Trace module initialized");
    ESP_LOGI(TAG, "Context switch logging enabled");
    return ESP_OK;
}

void trace_log_switch(const char *task_name, bool is_switch_in)
{
    int64_t timestamp_us = esp_timer_get_time();

    if (is_switch_in) {
        if (previous_task_name != NULL) {
            ESP_LOGI(TAG, "[%lld us] %s -> %s",
                     timestamp_us, previous_task_name, task_name);
        } else {
            ESP_LOGI(TAG, "[%lld us] -> %s", timestamp_us, task_name);
        }
        previous_task_name = task_name;
    }
}

void trace_print_stats(void)
{
    ESP_LOGI(TAG, "========== Task Runtime Stats ==========");

    // Get runtime stats
    vTaskGetRunTimeStats(stats_buffer);

    // Print header
    printf("Task            Abs Time        %% Time\n");
    printf("----------------------------------------\n");
    printf("%s", stats_buffer);

    ESP_LOGI(TAG, "========================================");

    // Also print task list with states
    ESP_LOGI(TAG, "========== Task States ==========");
    vTaskList(stats_buffer);
    printf("Task            State   Prio    Stack   Num\n");
    printf("--------------------------------------------\n");
    printf("%s", stats_buffer);
    ESP_LOGI(TAG, "=================================");
}

static void stats_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Stats task started - printing every %d ms", STATS_INTERVAL_MS);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATS_INTERVAL_MS));
        trace_print_stats();
    }
}

void trace_start_stats_task(void)
{
    xTaskCreate(stats_task, "trace_stats", 4096, NULL, 1, NULL);
    ESP_LOGI(TAG, "Stats task created");
}
