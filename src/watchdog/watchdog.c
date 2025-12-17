#include "watchdog.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "watchdog";

esp_err_t watchdog_init(uint32_t timeout_seconds)
{
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = timeout_seconds * 1000,
        .idle_core_mask = 0,  // Don't watch idle tasks
        .trigger_panic = true // Reset on timeout
    };

    // Try to reconfigure first (in case already initialized)
    esp_err_t ret = esp_task_wdt_reconfigure(&wdt_config);
    if (ret == ESP_ERR_INVALID_STATE)
    {
        ret = esp_task_wdt_init(&wdt_config);
    }

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Task watchdog initialized (timeout: %lus)", timeout_seconds);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to initialize watchdog: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t watchdog_register_task(void)
{
    esp_err_t ret = esp_task_wdt_add(NULL); // NULL = current task

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Task '%s' registered with watchdog", pcTaskGetName(NULL));
    }
    else if (ret == ESP_ERR_INVALID_ARG)
    {
        ESP_LOGW(TAG, "Task '%s' already registered", pcTaskGetName(NULL));
        ret = ESP_OK; // Not a fatal error
    }
    else
    {
        ESP_LOGE(TAG, "Failed to register task '%s': %s",
                 pcTaskGetName(NULL), esp_err_to_name(ret));
    }

    return ret;
}

void watchdog_feed(void)
{
    esp_task_wdt_reset();
}

void watchdog_unregister_task(void)
{
    esp_err_t ret = esp_task_wdt_delete(NULL);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Task '%s' unregistered from watchdog", pcTaskGetName(NULL));
    }
}
