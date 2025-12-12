/**
 * MQTT to SQLite Bridge
 * Driving Safety Monitor
 *
 * Subscribes to MQTT topics from the embedded device and stores
 * alerts and telemetry data in SQLite for analysis and visualization.
 *
 * Topics:
 *   - driving/alerts: Crash and warning events (QoS 1)
 *   - driving/telemetry: Batched sensor readings (QoS 0)
 *
 * JSON Formats:
 *   Alerts:  {"type":"warning","event":"harsh_braking","ts":12345,"x":0.1,"y":-4.5}
 *   Crash:   {"type":"crash","ts":12345,"mag":8.5}
 *   Batch:   {"ts":12345,"rate":100,"n":500,"d":[[x,y,z],[x,y,z],...]}
 */

const mqtt = require('mqtt');
const Database = require('better-sqlite3');
const path = require('path');

// Configuration
const config = {
    mqtt: {
        broker: 'mqtt://alderaan.software-engineering.ie',
        port: 1883,
        topics: [
            'driving/alerts',
            'driving/telemetry'
        ],
        options: {
            clientId: `driving-monitor-bridge-${Math.random().toString(16).slice(2, 8)}`,
            clean: true,
            connectTimeout: 4000,
            reconnectPeriod: 1000,
        }
    },
    sqlite: {
        filename: path.join(__dirname, 'driving_monitor.db')
    }
};

// Global connections
let db = null;
let mqttClient = null;

/**
 * Initialize SQLite database with new schema
 */
function initDatabase() {
    console.log(`[SQLite] Opening database: ${config.sqlite.filename}`);

    db = new Database(config.sqlite.filename);

    // Create tables for the new schema
    db.exec(`
        -- Alerts table: crashes and warnings
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type TEXT NOT NULL,           -- 'crash' or 'warning'
            event TEXT,                   -- 'harsh_braking' or 'harsh_acceleration' (for warnings)
            device_timestamp INTEGER,     -- timestamp from device (tick count)
            accel_magnitude REAL,         -- for crash events
            accel_x REAL,                 -- x acceleration (for warnings)
            accel_y REAL,                 -- y acceleration (for warnings)
            received_at INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        -- Telemetry batches metadata
        CREATE TABLE IF NOT EXISTS telemetry_batches (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_start_timestamp INTEGER NOT NULL,
            sample_rate_hz INTEGER NOT NULL,
            sample_count INTEGER NOT NULL,
            received_at INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        -- Individual sensor readings (expanded from batches)
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id INTEGER NOT NULL,
            sample_index INTEGER NOT NULL,
            calculated_timestamp INTEGER,  -- batch_start + (index * 1000 / sample_rate)
            x REAL NOT NULL,
            y REAL NOT NULL,
            z REAL NOT NULL,
            FOREIGN KEY (batch_id) REFERENCES telemetry_batches(id)
        );

        -- Indexes for efficient queries
        CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(type);
        CREATE INDEX IF NOT EXISTS idx_alerts_created ON alerts(created_at);
        CREATE INDEX IF NOT EXISTS idx_batches_created ON telemetry_batches(created_at);
        CREATE INDEX IF NOT EXISTS idx_readings_batch ON sensor_readings(batch_id);
        CREATE INDEX IF NOT EXISTS idx_readings_timestamp ON sensor_readings(calculated_timestamp);
    `);

    // Prepare insert statements
    db.insertAlert = db.prepare(`
        INSERT INTO alerts (type, event, device_timestamp, accel_magnitude, accel_x, accel_y, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    `);

    db.insertBatch = db.prepare(`
        INSERT INTO telemetry_batches (batch_start_timestamp, sample_rate_hz, sample_count, received_at)
        VALUES (?, ?, ?, ?)
    `);

    db.insertReading = db.prepare(`
        INSERT INTO sensor_readings (batch_id, sample_index, calculated_timestamp, x, y, z)
        VALUES (?, ?, ?, ?, ?, ?)
    `);

    // Batch insert for sensor readings (much faster)
    db.insertReadingsBatch = db.transaction((batchId, batchStartTimestamp, sampleRateHz, samples) => {
        for (let i = 0; i < samples.length; i++) {
            const calculatedTimestamp = batchStartTimestamp + Math.floor(i * 1000 / sampleRateHz);
            const [x, y, z] = samples[i];  // samples are [x, y, z] arrays
            db.insertReading.run(batchId, i, calculatedTimestamp, x, y, z);
        }
    });

    console.log('[SQLite] Database initialized with new schema');
    return db;
}

/**
 * Handle alert message (driving/alerts topic)
 *
 * Expected JSON format:
 * Warning: {"type":"warning","event":"harsh_braking","ts":12345,"x":0.1,"y":-4.5}
 * Crash:   {"type":"crash","ts":12345,"mag":8.5}
 */
function handleAlert(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();

    if (data.type === 'crash') {
        const result = db.insertAlert.run(
            'crash',
            null,
            data.ts,
            data.mag,
            null,
            null,
            receivedAt
        );
        console.log(`[Alert] CRASH detected! magnitude=${data.mag} (id: ${result.lastInsertRowid})`);
    } else if (data.type === 'warning') {
        const result = db.insertAlert.run(
            'warning',
            data.event,
            data.ts,
            null,
            data.x,
            data.y,
            receivedAt
        );
        console.log(`[Alert] WARNING: ${data.event} x=${data.x} y=${data.y} (id: ${result.lastInsertRowid})`);
    } else {
        console.warn(`[Alert] Unknown alert type: ${data.type}`);
    }
}

/**
 * Handle telemetry batch (driving/telemetry topic)
 *
 * Expected JSON format:
 * {"ts":12345,"rate":100,"n":500,"d":[[x,y,z],[x,y,z],...]}
 */
function handleTelemetry(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();

    // Insert batch metadata
    const batchResult = db.insertBatch.run(
        data.ts,
        data.rate,
        data.n,
        receivedAt
    );

    const batchId = batchResult.lastInsertRowid;

    // Insert all sensor readings in a transaction (fast)
    db.insertReadingsBatch(batchId, data.ts, data.rate, data.d);

    console.log(`[Telemetry] Batch stored: ${data.n} samples (batch_id: ${batchId})`);
}

/**
 * Connect to MQTT broker
 */
function connectMQTT() {
    console.log(`[MQTT] Connecting to ${config.mqtt.broker}...`);

    mqttClient = mqtt.connect(config.mqtt.broker, config.mqtt.options);

    mqttClient.on('connect', () => {
        console.log('[MQTT] Connected to broker');

        // Subscribe to alerts with QoS 1 (reliable)
        mqttClient.subscribe('driving/alerts', { qos: 1 }, (err) => {
            if (err) {
                console.error('[MQTT] Subscribe error for driving/alerts:', err.message);
            } else {
                console.log('[MQTT] Subscribed to: driving/alerts (QoS 1)');
            }
        });

        // Subscribe to telemetry with QoS 0 (fire and forget)
        mqttClient.subscribe('driving/telemetry', { qos: 0 }, (err) => {
            if (err) {
                console.error('[MQTT] Subscribe error for driving/telemetry:', err.message);
            } else {
                console.log('[MQTT] Subscribed to: driving/telemetry (QoS 0)');
            }
        });
    });

    mqttClient.on('message', (topic, message) => {
        try {
            if (topic === 'driving/alerts') {
                handleAlert(message);
            } else if (topic === 'driving/telemetry') {
                handleTelemetry(message);
            } else {
                console.log(`[MQTT] Unknown topic: ${topic}`);
            }
        } catch (error) {
            console.error(`[MQTT] Error handling message on ${topic}:`, error.message);
        }
    });

    mqttClient.on('error', (error) => {
        console.error('[MQTT] Error:', error.message);
    });

    mqttClient.on('reconnect', () => {
        console.log('[MQTT] Reconnecting...');
    });

    mqttClient.on('close', () => {
        console.log('[MQTT] Connection closed');
    });

    return mqttClient;
}

/**
 * Graceful shutdown
 */
function shutdown() {
    console.log('\n[System] Shutting down...');

    if (mqttClient) {
        mqttClient.end();
        console.log('[MQTT] Disconnected');
    }

    if (db) {
        db.close();
        console.log('[SQLite] Database closed');
    }

    process.exit(0);
}

/**
 * Main entry point
 */
function main() {
    console.log('='.repeat(50));
    console.log('  Driving Safety Monitor - MQTT to SQLite Bridge');
    console.log('='.repeat(50));

    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);

    try {
        initDatabase();
        connectMQTT();
        console.log('[System] Bridge is running. Press Ctrl+C to stop.');
    } catch (error) {
        console.error('[System] Startup error:', error.message);
        process.exit(1);
    }
}

main();
