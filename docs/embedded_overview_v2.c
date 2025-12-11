// In sensor_task: accumulate samples into batches
static sensor_batch_t current_batch;
static uint16_t batch_index = 0;

// Task runs every 10ms, processing task runs every 100ms
// Buffer of up to 10 samples (100ms) if briefly delayed.
// Unlikely to be delayed
#define SENSOR_QUEUE_SIZE 10

// Reduced from 50 - now only handles exceptional events
// Bulk logging offloaded to batch_queue
// 20 messages = 4-10 seconds of continuous alerts during network issues
#define MQTT_QUEUE_SIZE 20

// Infrequent, processed quickly, not time critical.
// Unlikely to be used
#define COMMAND_QUEUE_SIZE 5

// Small buffer for bulk telemetry data
// 3 batches × 6KB = 18KB, provides 30 seconds of buffering at 10s intervals
#define BATCH_QUEUE_SIZE 3

// Batch configuration
#define BATCH_SIZE 500     // 500 samples per batch
#define SAMPLE_RATE_HZ 100 // 100Hz sampling

// Consideration: QoS 1/2 for Warnings/Crash, 0 for logs
// Consideration: sizeof(sensor_data_t) × 10 + sizeof(mqtt_message_t) × 20 +
// sizeof(sensor_batch_t) × 3 + sizeof(command_t) × 5 must be < RAM

QueueHandle_t sensor_queue;
QueueHandle_t batch_queue;
QueueHandle_t mqtt_queue;
QueueHandle_t command_queue;

typedef struct {
  uint32_t batch_start_timestamp;       // Only one timestamp per batch
  uint16_t sample_rate_hz;              // e.g., 50 or 100
  uint16_t sample_count;                // Number of samples in this batch
  sensor_reading_t samples[BATCH_SIZE]; // Just the sensor values
} sensor_batch_t;

typedef struct {
  float x;
  float y;
  float z;
} sensor_reading_t; // 12 bytes vs 28+ with timestamp

static sensor_batch_t current_batch;
static uint16_t batch_index = 0;

void main() {
  init_device();
  init_imu();
  init_wifi();

  sensor_queue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(sensor_reading_t));
  batch_queue = xQueueCreate(BATCH_QUEUE_SIZE, sizeof(sensor_batch_t));
  mqtt_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_message_t));
  command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(command_t));

  // Initialize first batch
  current_batch.batch_start_timestamp = xTaskGetTickCount();
  current_batch.sample_rate_hz = SAMPLE_RATE_HZ;
  batch_index = 0;

  xTaskCreate(mqtt_task, "mqtt", 8192, NULL, 2, NULL);
  xTaskCreate(processing_task, "process", 4096, NULL, 4, NULL);
  xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
}

// Lowest Priority
void mqtt_task(void *pvParameters) {
  mqtt_message_t alert_msg;
  sensor_batch_t batch;

  while (1) {
    // Check for critical alerts first (non-blocking)
    // Always handle alerts immediately - small, urgent messages
    while (xQueueReceive(mqtt_queue, &alert_msg, 0) == pdTRUE) {
      send_alert_immediately(&alert_msg);
    }

    // Send batched data if no alerts pending
    // Large messages that can wait
    if (xQueueReceive(batch_queue, &batch, pdMS_TO_TICKS(1000)) == pdTRUE) {
      send_batch_to_server(&batch);
    }
  }
}

// Highest priority: Priority #1
// Note: The pvParameters parameter is what makes this a task
void sensor_task(void *pvParameters) {
  sensor_reading_t data;
  TickType_t last_wake_time = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(10); // 100Hz

  while (1) {
    // Read 6DOF accelerometer/gyroscope (I2C)
    data.x = read_accel_x();
    data.y = read_accel_y();
    data.z = read_accel_z();

    // Send to processing task (non-blocking)
    xQueueSend(sensor_queue, &data, 0);

    // Precise timing for real-time sampling
    vTaskDelayUntil(&last_wake_time, period);
  }
}

// Priority #2
void processing_task(void *pvParameters) {
  sensor_reading_t sensor_data;
  command_t cmd;

  while (1) {
    // Process all pending commands (non-blocking)
    //
    // How to read this:
    // Because we pass in 0, FreeRTOS will not wait at all if there is nothing
    // in the queue. It will just skip the loop. The while loop handles each
    // command in the queue, which is not blocking
    //
    // It also makes sense to handle commands more readily than sensor data,
    // since setting the threshold will affect how we react to the sensor data.
    while (xQueueReceive(command_queue, &cmd, 0) == pdTRUE) {
      handle_command(&cmd);
    }

    // Wait for sensor data (with timeout)
    if (xQueueReceive(sensor_queue, &sensor_data, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      process_sensor_data(&sensor_data);
    }
  }
}

static void process_sensor_data(const sensor_data_t *data) {
  float accel_magnitude = calculate_accel_magnitude(data);

  if (detect_crash(accel_magnitude, data)) {
    handle_crash(data);
  }

  if (detect_harsh_braking(accel_magnitude, data)) {
    handle_harsh_braking(data);
  }

  if (detect_harsh_acceleration(accel_magnitude, data)) {
    handle_harsh_acceleration(data);
  }

  send_log(data, accel_magnitude);
}

void send_log(const sensor_data_t *data, float accel_magnitude) {
  sensor_sample_t *s = &current_batch.samples[batch_index];
  s->x = data.x;
  s->y = data.y;
  s->z = data.z;

  batch_index++;

  // When batch is full (every 5 seconds at 100Hz)
  if (batch_index >= BATCH_SIZE) {
    current_batch.sample_count = batch_index;

    // Send batch (non-blocking to avoid delaying sensor processing)
    xQueueSend(batch_queue, &current_batch, 0);

    // Prepare the next batch
    batch_index = 0;
    current_batch.batch_start_timestamp = xTaskGetTickCount();
  }
}

static bool detect_harsh_braking(const sensor_data_t *data) {
  return data->accel_y > harsh_accel_threshold;
}

static void handle_harsh_braking(float accel_magnitude,
                                 const sensor_data_t *data) {
  mqtt_message_t msg;
  msg.type = MSG_WARNING;
  msg.data.warning.event = WARNING_HARSH_BRAKING;
  msg.data.warning.timestamp = data->timestamp; // Full timestamp for alerts
  msg.data.warning.accel_y = data->accel_y;

  // Send immediately via mqtt_queue - don't wait for batch
  xQueueSend(mqtt_queue, &msg, 0);
}

static void handle_crash(float accel_magnitude, const sensor_data_t *data) {
  mqtt_message_t msg;
  msg.type = MSG_CRASH;
  // Full timestamp for critical events
  msg.data.crash.timestamp = data->timestamp;
  msg.data.crash.accel_magnitude = accel_magnitude;

  // Critical event - send immediately
  xQueueSend(mqtt_queue, &msg, 0);
}

static void handle_harsh_acceleration(float accel_magnitude,
                                      const sensor_data_t *data) {
  mqtt_message_t msg;
  msg.type = MSG_WARNING;
  msg.data.warning.event = WARNING_HARSH_ACCEL;
  // Full timestamp for alerts
  msg.data.warning.timestamp = data->timestamp;
  msg.data.warning.accel_y = data->accel_y;

  // Send immediately via mqtt_queue
  xQueueSend(mqtt_queue, &msg, 0);
}
