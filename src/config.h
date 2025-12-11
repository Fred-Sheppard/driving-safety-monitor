#ifndef CONFIG_H
#define CONFIG_H

// Uncomment to use mock sensor data instead of real IMU
// #define MOCK_SENSOR_DATA

#define IMU_SAMPLE_RATE_HZ 100
#define SENSOR_INTERVAL_MS (1000 / IMU_SAMPLE_RATE_HZ)

#define SENSOR_QUEUE_SIZE 10
#define MQTT_QUEUE_SIZE 20
#define COMMAND_QUEUE_SIZE 5
#define BATCH_QUEUE_SIZE 3

#define LOG_BATCH_SIZE 500

#define SENSOR_TASK_PRIORITY 5
#define PROCESSING_TASK_PRIORITY 4
#define MQTT_TASK_PRIORITY 2

#define WIFI_SSID "JoshiPhone"
#define WIFI_PASSWORD "joshshotspot"
#define WIFI_MAXIMUM_RETRY 10

#define MQTT_BROKER_URI "mqtt://alderaan.software-engineering.ie:1883"
#define MQTT_TOPIC_ALERTS "driving/alerts"
#define MQTT_TOPIC_TELEMETRY "driving/telemetry"
#define MQTT_QOS_ALERTS 1    // At-least-once for critical alerts
#define MQTT_QOS_TELEMETRY 0 // At-most-once for bulk telemetry

#define DEFAULT_CRASH_THRESHOLD_G 8.0f
#define DEFAULT_HARSH_BRAKING_THRESHOLD_G 4.0f
#define DEFAULT_HARSH_ACCEL_THRESHOLD_G 3.5f
#define DEFAULT_HARSH_CORNERING_THRESHOLD_G 3.0f

#endif // CONFIG_H
