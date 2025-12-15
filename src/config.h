#ifndef CONFIG_H
#define CONFIG_H

// Local configuration not tracked by git
#include "config_local.h"

#ifndef WIFI_SSID
#error "WIFI_SSID must be defined in config_local.h"
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD must be defined in config_local.h"
#endif

#ifndef DEVICE_NAME
#define DEVICE_NAME "DriveMonitor"
#endif

#define IMU_SAMPLE_RATE_HZ 100
#define SENSOR_INTERVAL_MS (1000 / IMU_SAMPLE_RATE_HZ)

#define SENSOR_QUEUE_SIZE 10
#define MQTT_QUEUE_SIZE 20
#define COMMAND_QUEUE_SIZE 5
#define BATCH_QUEUE_SIZE 3

#define LOG_BATCH_SIZE 500

#define SENSOR_TASK_PRIORITY 5
#define SCREEN_TASK_PRIORITY 3
#define PROCESSING_TASK_PRIORITY 4
#define MQTT_TASK_PRIORITY 2

#define WIFI_MAXIMUM_RETRY 10

#define MQTT_BROKER_URI "mqtt://alderaan.software-engineering.ie:1883"
#define MQTT_TOPIC_ALERTS "driving/alerts"
#define MQTT_TOPIC_TELEMETRY "driving/telemetry"
#define MQTT_TOPIC_COMMANDS "driving/commands"
#define MQTT_TOPIC_STATUS "driving/status"
#define MQTT_QOS_ALERTS 1
#define MQTT_QOS_TELEMETRY 0
#define MQTT_QOS_COMMANDS 1
#define MQTT_QOS_STATUS 1

#define DEFAULT_CRASH_THRESHOLD_G 11.0f
#define DEFAULT_HARSH_BRAKING_THRESHOLD_G 9.0f
#define DEFAULT_HARSH_ACCEL_THRESHOLD_G 7.0f
#define DEFAULT_HARSH_CORNERING_THRESHOLD_G 8.0f

#ifndef TRACE_CONTEXT_SWITCHES
#define TRACE_CONTEXT_SWITCHES 0
#endif

#ifndef TRACE_STATS_ENABLED
#define TRACE_STATS_ENABLED 0
#endif

#ifndef TRACE_STATS_INTERVAL_MS
#define TRACE_STATS_INTERVAL_MS 5000
#endif

#endif