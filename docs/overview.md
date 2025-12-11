## Sampling, Batching, and Transmission Strategy

**My recommendations:**

**(a) Collect data: 50-100Hz (10-20ms intervals)**
- Your current 100Hz is good for detecting rapid events (crashes, hard braking)
- 50Hz is usually sufficient for automotive applications and reduces processing load
- Don't go below 20Hz - you'll miss important transient events

**(b) Send batched data: Every 5-10 seconds**
- Balances data freshness with network efficiency
- Reduces WiFi radio-on time (saves power)
- Limits MQTT overhead (connection maintenance, packet headers)
- Still provides near real-time monitoring for your server dashboard

**(c) Batch size: 250-500 samples**
- At 50Hz × 10 seconds = 500 samples per batch
- At 100Hz × 5 seconds = 500 samples per batch
- Keeps individual MQTT messages under 10-20KB (manageable for most brokers)
- Provides good granularity for server-side graphing

## Queue Interaction Architecture

**You now have two types of data flowing to the MQTT task:**

```c
// Two separate queues feeding the MQTT task
QueueHandle_t mqtt_queue;      // High-priority alerts (crashes, warnings)
QueueHandle_t batch_queue;     // Bulk telemetry data (for graphing)

void mqtt_task(void *pvParameters) {
    mqtt_message_t alert_msg;
    sensor_batch_t batch;
    
    while (1) {
        // PRIORITY 1: Check for critical alerts first (non-blocking)
        while (xQueueReceive(mqtt_queue, &alert_msg, 0) == pdTRUE) {
            send_alert_immediately(&alert_msg);  // Small, urgent
        }
        
        // PRIORITY 2: Send batched data if no alerts pending
        if (xQueueReceive(batch_queue, &batch, pdMS_TO_TICKS(1000)) == pdTRUE) {
            send_batch_to_server(&batch);  // Large, can wait
        }
    }
}
```

## Recommended Queue Sizes

```c
mqtt_queue = xQueueCreate(20, sizeof(mqtt_message_t));     // Reduced from 50
batch_queue = xQueueCreate(3, sizeof(sensor_batch_t));     // Small buffer
```

### mqtt_queue (20 items) - Down from 50

**Why smaller now:**
- You've offloaded the bulk logging data to `batch_queue`
- This queue now only handles **exceptional events**: crashes, harsh braking, threshold violations
- Even in aggressive driving, you might generate 2-5 events per second max
- 20 messages = 4-10 seconds of continuous alerts during network issues
- Most `mqtt_message_t` structs are small (50-200 bytes typically)

**Why not smaller:**
- Still need resilience during WiFi reconnection (2-5 seconds common)
- If multiple events happen during a crash sequence, all should be buffered

### batch_queue (3 items)

**Why only 3:**
- Each batch is **large** (500 samples × 12 bytes = 6KB+ per batch)
- At 10-second batch intervals, 3 batches = 30 seconds of buffering
- If MQTT is down for 30+ seconds, you have bigger problems
- Provides natural backpressure: sensor task blocks when pool exhausted (prevents memory exhaustion)

**Memory calculation:**
```
3 batches × 6KB = 18KB for batch_queue
20 alerts × 100 bytes = 2KB for mqtt_queue
Total: ~20KB queue memory (very reasonable)
```

## Timestamp Strategy

**Skip individual timestamps - use calculated timestamps instead.**

**Benefits:**
- **Massive space savings**: Each timestamp is 4 bytes (uint32_t). For 500 samples, that's 2KB saved per batch
- **Reduced transmission cost**: With cellular/metered connections, this matters
- **Simpler serialization**: Just send array of sensor values
- **Better compression**: Repeated data patterns compress poorly; uniform sensor data compresses well

**Implementation:**
```c
typedef struct {
    uint32_t batch_start_timestamp;  // Only one timestamp per batch
    uint16_t sample_rate_hz;          // e.g., 50 or 100
    uint16_t sample_count;            // Number of samples in this batch
    sensor_reading_t samples[];       // Just the sensor values
} sensor_batch_t;

typedef struct {
    int16_t accel_x;  // Can use int16_t for space savings
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} sensor_reading_t;  // 12 bytes vs 28+ with timestamp
```

**Server reconstructs timestamps:**
```python
timestamp_ms = batch_start_timestamp + (sample_index * 1000 / sample_rate_hz)
```

**Complications vs Benefits:**

*Complications:*
- Clock drift over long periods (minor - sync periodically with NTP)
- If a sample is dropped, subsequent timestamps shift (mitigate: never drop samples within a batch)
- Slightly more complex server-side processing

*Benefits:*
- 40-60% reduction in data size per batch
- Lower bandwidth costs (crucial for cellular)
- Reduced queue memory requirements
- Faster transmission = better battery life

**Verdict: The benefits strongly outweigh complications.** This is standard practice in embedded data logging systems.

**For critical events** (crashes, harsh braking), send immediately via the existing `mqtt_queue` with full timestamp - don't wait for the batch window.