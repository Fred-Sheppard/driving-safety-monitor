/**
 * MQTT-SQLite Bridge with Dashboard Server
 *
 * Combines the MQTT bridge with an Express server that:
 * - Serves a React dashboard
 * - Provides REST API endpoints for querying data
 */

const mqtt = require('mqtt');
const Database = require('better-sqlite3');
const express = require('express');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3001;

// Configuration
const config = {
    mqtt: {
        broker: 'mqtt://alderaan.software-engineering.ie',
        port: 1883,
        topics: ['driving/alerts', 'driving/telemetry'],
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
let batchCounter = 0;

// ============== Database Setup ==============

function initDatabase() {
    console.log(`[SQLite] Opening database: ${config.sqlite.filename}`);
    db = new Database(config.sqlite.filename);

    db.exec(`
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type TEXT NOT NULL,
            event TEXT,
            device_timestamp INTEGER,
            accel_magnitude REAL,
            accel_x REAL,
            accel_y REAL,
            received_at INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id INTEGER NOT NULL,
            sample_index INTEGER NOT NULL,
            batch_start_timestamp INTEGER,
            sample_rate_hz INTEGER,
            calculated_timestamp INTEGER,
            x REAL NOT NULL,
            y REAL NOT NULL,
            z REAL NOT NULL,
            received_at INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(type);
        CREATE INDEX IF NOT EXISTS idx_alerts_created ON alerts(created_at);
        CREATE INDEX IF NOT EXISTS idx_readings_batch ON sensor_readings(batch_id);
        CREATE INDEX IF NOT EXISTS idx_readings_timestamp ON sensor_readings(calculated_timestamp);
    `);

    const maxBatch = db.prepare('SELECT MAX(batch_id) as max FROM sensor_readings').get();
    batchCounter = (maxBatch.max || 0) + 1;

    db.insertAlert = db.prepare(`
        INSERT INTO alerts (type, event, device_timestamp, accel_magnitude, accel_x, accel_y, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    `);

    db.insertReading = db.prepare(`
        INSERT INTO sensor_readings (batch_id, sample_index, batch_start_timestamp, sample_rate_hz, calculated_timestamp, x, y, z, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    `);

    db.insertReadingsBatch = db.transaction((batchId, batchStartTimestamp, sampleRateHz, samples, receivedAt) => {
        for (let i = 0; i < samples.length; i++) {
            const calculatedTimestamp = batchStartTimestamp + Math.floor(i * 1000 / sampleRateHz);
            const [x, y, z] = samples[i];
            db.insertReading.run(batchId, i, batchStartTimestamp, sampleRateHz, calculatedTimestamp, x, y, z, receivedAt);
        }
    });

    console.log(`[SQLite] Database initialized (next batch_id: ${batchCounter})`);
}

// ============== MQTT Handlers ==============

function handleAlert(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();

    if (data.type === 'crash') {
        db.insertAlert.run('crash', null, data.ts, data.mag, null, null, receivedAt);
        console.log(`[Alert] CRASH detected! magnitude=${data.mag}`);
    } else if (data.type === 'warning') {
        db.insertAlert.run('warning', data.event, data.ts, null, data.x, data.y, receivedAt);
        console.log(`[Alert] WARNING: ${data.event}`);
    }
}

function handleTelemetry(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();
    const batchId = batchCounter++;
    db.insertReadingsBatch(batchId, data.ts, data.rate, data.d, receivedAt);
    console.log(`[Telemetry] Batch stored: ${data.n} samples (batch_id: ${batchId})`);
}

function connectMQTT() {
    console.log(`[MQTT] Connecting to ${config.mqtt.broker}...`);
    mqttClient = mqtt.connect(config.mqtt.broker, config.mqtt.options);

    mqttClient.on('connect', () => {
        console.log('[MQTT] Connected to broker');
        mqttClient.subscribe('driving/alerts', { qos: 1 });
        mqttClient.subscribe('driving/telemetry', { qos: 0 });
    });

    mqttClient.on('message', (topic, message) => {
        try {
            if (topic === 'driving/alerts') handleAlert(message);
            else if (topic === 'driving/telemetry') handleTelemetry(message);
        } catch (error) {
            console.error(`[MQTT] Error:`, error.message);
        }
    });

    mqttClient.on('error', (err) => console.error('[MQTT] Error:', err.message));
    mqttClient.on('reconnect', () => console.log('[MQTT] Reconnecting...'));
}

// ============== REST API ==============

app.use(express.json());

// Get recent alerts
app.get('/api/alerts', (req, res) => {
    const limit = parseInt(req.query.limit) || 50;
    const alerts = db.prepare(`
        SELECT * FROM alerts ORDER BY created_at DESC LIMIT ?
    `).all(limit);
    res.json(alerts);
});

// Get alert counts by type
app.get('/api/alerts/summary', (req, res) => {
    const summary = db.prepare(`
        SELECT type, event, COUNT(*) as count
        FROM alerts
        GROUP BY type, event
    `).all();
    res.json(summary);
});

// Get recent sensor batches (metadata only)
app.get('/api/batches', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const batches = db.prepare(`
        SELECT batch_id, batch_start_timestamp, sample_rate_hz,
               COUNT(*) as sample_count, MIN(created_at) as created_at
        FROM sensor_readings
        GROUP BY batch_id
        ORDER BY batch_id DESC
        LIMIT ?
    `).all(limit);
    res.json(batches);
});

// Get sensor readings for a specific batch
app.get('/api/batches/:batchId', (req, res) => {
    const readings = db.prepare(`
        SELECT sample_index, x, y, z, calculated_timestamp
        FROM sensor_readings
        WHERE batch_id = ?
        ORDER BY sample_index
    `).all(req.params.batchId);
    res.json(readings);
});

// Get latest N readings (for live chart)
app.get('/api/readings/latest', (req, res) => {
    const limit = parseInt(req.query.limit) || 500;
    const readings = db.prepare(`
        SELECT x, y, z, calculated_timestamp, batch_id, sample_index
        FROM sensor_readings
        ORDER BY batch_id DESC, sample_index DESC
        LIMIT ?
    `).all(limit);
    res.json(readings.reverse());
});

// Get stats
app.get('/api/stats', (req, res) => {
    const alertCount = db.prepare('SELECT COUNT(*) as count FROM alerts').get();
    const crashCount = db.prepare("SELECT COUNT(*) as count FROM alerts WHERE type='crash'").get();
    const readingCount = db.prepare('SELECT COUNT(*) as count FROM sensor_readings').get();
    const batchCount = db.prepare('SELECT COUNT(DISTINCT batch_id) as count FROM sensor_readings').get();

    res.json({
        totalAlerts: alertCount.count,
        crashes: crashCount.count,
        warnings: alertCount.count - crashCount.count,
        totalReadings: readingCount.count,
        totalBatches: batchCount.count
    });
});

// Send command to device via MQTT
app.post('/api/config/threshold', (req, res) => {
    const { type, value } = req.body;

    const validTypes = ['crash', 'braking', 'accel', 'cornering'];
    if (!validTypes.includes(type)) {
        return res.status(400).json({ error: 'Invalid threshold type' });
    }

    if (typeof value !== 'number' || value < 0 || value > 50) {
        return res.status(400).json({ error: 'Value must be a number between 0 and 50' });
    }

    const command = JSON.stringify({
        cmd: 'set_threshold',
        type: type,
        value: value
    });

    mqttClient.publish('driving/commands', command, { qos: 1 }, (err) => {
        if (err) {
            console.error('[MQTT] Failed to publish command:', err.message);
            return res.status(500).json({ error: 'Failed to send command' });
        }
        console.log(`[Config] Sent threshold command: ${type}=${value}G`);
        res.json({ success: true, type, value });
    });
});

// ============== Dashboard ==============

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'dashboard.html'));
});

// ============== Startup ==============

function shutdown() {
    console.log('\n[System] Shutting down...');
    if (mqttClient) mqttClient.end();
    if (db) db.close();
    process.exit(0);
}

function main() {
    console.log('='.repeat(50));
    console.log('  Driving Safety Monitor - Dashboard Server');
    console.log('='.repeat(50));

    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);

    initDatabase();
    connectMQTT();

    app.listen(PORT, () => {
        console.log(`[HTTP] Dashboard running at http://localhost:${PORT}`);
    });
}

main();
