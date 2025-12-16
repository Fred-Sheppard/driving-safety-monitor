#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initialize and start the task watchdog timer
 * @param timeout_seconds Timeout before system reset
 * @return ESP_OK on success
 */
esp_err_t watchdog_init(uint32_t timeout_seconds);

/**
 * @brief Register current task with watchdog
 * Call this at the start of each task you want to monitor
 * @return ESP_OK on success
 */
esp_err_t watchdog_register_task(void);

/**
 * @brief Feed the watchdog - call periodically from task loop
 * Must be called more frequently than the timeout
 */
void watchdog_feed(void);

/**
 * @brief Unregister task from watchdog
 * Call before task deletion if ever needed
 */
void watchdog_unregister_task(void);

#endif // WATCHDOG_H
