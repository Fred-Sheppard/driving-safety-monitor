#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t watchdog_init(uint32_t timeout_seconds);

// Call at start of each monitored task
esp_err_t watchdog_register_task(void);

void watchdog_feed(void);

void watchdog_unregister_task(void);

#ifdef __cplusplus
}
#endif

#endif // WATCHDOG_H
