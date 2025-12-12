#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "message_types.h"

/**
 * @brief Serialize alert message to JSON
 * @param msg Alert message structure
 * @return Pointer to static buffer (valid until next call), or NULL on error
 */
const char *serialize_alert(const mqtt_message_t *msg);

/**
 * @brief Serialize batch telemetry to JSON
 * @param batch Sensor batch structure
 * @return Pointer to static buffer (valid until next call), or NULL on error
 */
const char *serialize_batch(const sensor_batch_t *batch);

#endif // SERIALIZE_H
