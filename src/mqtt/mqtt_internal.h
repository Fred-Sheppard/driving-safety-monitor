#ifndef MQTT_INTERNAL_H
#define MQTT_INTERNAL_H

#include "mqtt_client.h"
#include "message_types.h"
#include <stdbool.h>

// Shared MQTT client handle
extern esp_mqtt_client_handle_t g_mqtt_client;

// Connection state
extern bool g_mqtt_connected;
extern bool g_status_requested;

// Command handling (mqtt_commands.c)
void mqtt_handle_command(const char *data, int data_len);
void mqtt_publish_status(const threshold_status_t *status);

#endif // MQTT_INTERNAL_H
