# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Embedded driving safety monitor using ESP32 (FireBeetle32 board) with ESP-IDF framework. The system collects IMU sensor data at 100Hz, detects safety events (crashes, harsh braking/acceleration), and transmits data via MQTT over WiFi.

## Build Commands

```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Build and upload
pio run --target upload && pio device monitor

# Clean build
pio run --target clean
```

## Architecture

**FreeRTOS Task Model (3 tasks with priority-based scheduling):**

1. **sensor_task** (Priority 5 - highest): Reads 6DOF accelerometer/gyroscope at 100Hz via I2C, sends to processing queue
2. **processing_task** (Priority 4): Processes sensor data, detects events (crash/harsh braking/acceleration), accumulates samples into batches
3. **mqtt_task** (Priority 2 - lowest): Sends critical alerts immediately, sends batched telemetry data

**Queue Architecture:**
- `sensor_queue` (10 items): Raw sensor readings from sensor_task to processing_task
- `mqtt_queue` (20 items): High-priority alerts (crashes, warnings) - sent immediately
- `batch_queue` (3 items): Bulk telemetry batches (500 samples each) - sent every 5 seconds
- `command_queue` (5 items): Remote commands from server

**Data Flow:**
- Alerts (crashes, harsh events) → `mqtt_queue` → immediate transmission with full timestamp
- Telemetry → accumulated in `sensor_batch_t` → `batch_queue` → bulk transmission (timestamps reconstructed server-side from batch_start_timestamp + sample_rate)

## Key Constants

- `SAMPLE_RATE_HZ`: 100 (10ms sampling interval)
- `BATCH_SIZE`: 500 samples per batch
- Batch transmission interval: ~5 seconds (500 samples / 100Hz)
