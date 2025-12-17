#ifndef MQTT_INTERNAL_H
#define MQTT_INTERNAL_H

#include "mqtt_client.h"
#include "message_types.h"
#include <stdbool.h>

#define DEVICE_ID_LEN 13
extern char g_device_id[DEVICE_ID_LEN];

extern esp_mqtt_client_handle_t g_mqtt_client;
extern bool g_mqtt_connected;
extern bool g_status_requested;

void mqtt_handle_command(const char *data, int data_len);
void mqtt_publish_status(const threshold_status_t *status);

#endif // MQTT_INTERNAL_H
