# Backend Architecture

The dashboard server is an MQTT-SQLite bridge that receives telemetry from ESP32 devices, stores it in a SQLite database, and serves a React dashboard via REST API.

## Directory Structure

```
docs/mqtt-sqlite-bridge/
├── server.js              # Entry point
├── dashboard.html         # React frontend (single file)
├── package.json
├── driving_monitor.db     # SQLite database (auto-created)
└── src/
    ├── config.js          # Configuration constants
    ├── database.js        # SQLite operations
    ├── devices.js         # In-memory device tracking
    ├── mqtt.js            # MQTT client and handlers
    └── routes/
        ├── alerts.js      # /api/alerts endpoints
        ├── devices.js     # /api/devices endpoints
        ├── readings.js    # /api/readings, /api/batches endpoints
        └── stats.js       # /api/stats, /api/score endpoints
```

## Module Responsibilities

### config.js
Central configuration for MQTT broker, topics, QoS levels, database path, and default thresholds.

### database.js
All SQLite operations using better-sqlite3:
- Schema initialization (alerts, sensor_readings tables)
- Prepared statements for inserts
- Query functions for alerts, readings, batches, stats
- Transaction support for batch inserts

### devices.js
In-memory Map tracking connected devices:
- Device ID (MAC address)
- Connection status
- Current thresholds (synced from device)
- Last update timestamp

### mqtt.js
MQTT client lifecycle and message handlers:
- Connects to broker, subscribes to topics
- Routes messages to appropriate handlers
- Publishes commands to device-specific topics

## Data Flow

```
┌─────────────┐     MQTT      ┌─────────────┐     SQLite    ┌──────────┐
│   ESP32     │──────────────▶│   Server    │──────────────▶│ Database │
│   Device    │               │  (mqtt.js)  │               │          │
└─────────────┘               └─────────────┘               └──────────┘
       ▲                             │
       │                             │ REST API
       │                             ▼
       │                      ┌─────────────┐
       │      MQTT Command    │  Dashboard  │
       └──────────────────────│  (React)    │
                              └─────────────┘
```

## MQTT Topics

| Topic | Direction | QoS | Description |
|-------|-----------|-----|-------------|
| `driving/alerts` | Device → Server | 1 | Crash and warning events |
| `driving/telemetry` | Device → Server | 0 | Batched accelerometer data |
| `driving/status` | Device → Server | 1 | Current threshold values |
| `driving/commands/{deviceId}` | Server → Device | 1 | Threshold configuration |

## Message Formats

### Alert (crash)
```json
{"dev":"A1B2C3D4E5F6","type":"crash","ts":12345678,"mag":12.5}
```

### Alert (warning)
```json
{"dev":"A1B2C3D4E5F6","type":"warning","event":"harsh_braking","ts":12345678,"x":0.1,"y":-9.5}
```

### Telemetry
```json
{"dev":"A1B2C3D4E5F6","ts":12345678,"rate":100,"n":500,"d":[[0.1,0.2,9.8],...]}
```

### Status
```json
{"dev":"A1B2C3D4E5F6","crash":11.0,"braking":9.0,"accel":7.0,"cornering":8.0}
```

### Command
```json
{"cmd":"set_threshold","type":"crash","value":12.0}
```

## REST API

### Devices
- `GET /api/devices` - List all known devices
- `GET /api/devices/:id/status` - Get device status and thresholds
- `POST /api/devices/:id/threshold` - Send threshold command

### Alerts
- `GET /api/alerts?device=&limit=` - Get recent alerts
- `GET /api/alerts/summary?device=` - Get alert counts by type
- `GET /api/alerts/history?device=&hours=` - Get hourly event histogram

### Readings
- `GET /api/readings/latest?device=&limit=` - Get latest sensor readings
- `GET /api/batches?device=&limit=` - Get batch metadata
- `GET /api/batches/:id` - Get samples for a batch

### Stats
- `GET /api/stats?device=` - Get aggregate statistics
- `POST /api/score/reset?device=` - Clear alerts (reset driving score)

## Database Schema

### alerts
| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER | Primary key |
| device_id | TEXT | Device MAC address |
| type | TEXT | 'crash' or 'warning' |
| event | TEXT | Warning event type (nullable) |
| device_timestamp | INTEGER | Device-side timestamp |
| accel_magnitude | REAL | Crash magnitude (nullable) |
| accel_x, accel_y | REAL | Warning acceleration (nullable) |
| received_at | INTEGER | Server receive timestamp |
| created_at | DATETIME | Row creation time |

### sensor_readings
| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER | Primary key |
| device_id | TEXT | Device MAC address |
| batch_id | INTEGER | Batch identifier |
| sample_index | INTEGER | Sample position in batch |
| batch_start_timestamp | INTEGER | Batch start time |
| sample_rate_hz | INTEGER | Sampling rate |
| calculated_timestamp | INTEGER | Computed sample time |
| x, y, z | REAL | Accelerometer values |
| received_at | INTEGER | Server receive timestamp |
| created_at | DATETIME | Row creation time |

## Driving Score Calculation

The driving score is a ratio-based metric:

```javascript
score = 100 * e^(-eventsPerBatch * 2)
```

Where:
- `eventsPerBatch = (crashes * 5 + warnings) / totalBatches`
- Each batch represents ~5 seconds of driving
- Crashes are weighted 5x more than warnings
- Score naturally recovers as clean batches accumulate

## Running the Server

```bash
cd docs/mqtt-sqlite-bridge
npm install
npm start
```

Dashboard available at `http://localhost:3001`
