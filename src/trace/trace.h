#ifndef TRACE_H
#define TRACE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Enable/disable verbose context switch logging
#define TRACE_CONTEXT_SWITCHES 0

// Macro to trace task execution - place at start of task loop
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

// Macro to trace specific events
#define TRACE_EVENT(tag, fmt, ...)           \
    ESP_LOGI("TRACE", "[%lld us] [%s] " fmt, \
             esp_timer_get_time(), tag, ##__VA_ARGS__)

    /**
     * Initialize the trace module
     * Call this after creating all tasks you want to monitor
     */
    esp_err_t trace_init(void);

    /**
     * Start the trace stats task
     * Periodically prints task runtime statistics
     */
    void trace_start_stats_task(void);

    /**
     * Print current task runtime stats to serial
     */
    void trace_print_stats(void);

    /**
     * Log a context switch (call from trace hooks if needed)
     */
    void trace_log_switch(const char *task_name, bool is_switch_in);

#ifdef __cplusplus
}
#endif

#endif // TRACE_H
