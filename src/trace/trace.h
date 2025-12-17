#ifndef TRACE_H
#define TRACE_H

#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Place at start of task loop
#if TRACE_CONTEXT_SWITCHES
#define TRACE_TASK_RUN(tag)                                        \
    do                                                             \
    {                                                              \
        static int64_t _last_trace_time = 0;                       \
        int64_t _now = esp_timer_get_time();                       \
        if (_now - _last_trace_time > 100000)                      \
        { /* Log max every 100ms */                                \
            ESP_LOGI("TRACE", "[%lld us] %s running on core %d",   \
                     _now, pcTaskGetName(NULL), xPortGetCoreID()); \
            _last_trace_time = _now;                               \
        }                                                          \
    } while (0)
#else
#define TRACE_TASK_RUN(tag) ((void)0)
#endif

#define TRACE_EVENT(tag, fmt, ...)           \
    ESP_LOGI("TRACE", "[%lld us] [%s] " fmt, \
             esp_timer_get_time(), tag, ##__VA_ARGS__)

    // Call after creating tasks
    esp_err_t trace_init(void);

    void trace_start_stats_task(void);
    void trace_print_stats(void);
    void trace_log_switch(const char *task_name, bool is_switch_in);

#ifdef __cplusplus
}
#endif

#endif // TRACE_H
