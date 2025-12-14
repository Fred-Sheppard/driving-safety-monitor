/**
 * MQTT-SQLite Bridge with Dashboard Server
 * Multi-device support - tracks multiple ESP32 devices
 */

const mqtt = require('mqtt');
const Database = require('better-sqlite3');
const express = require('express');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3001;

const config = {
    mqtt: {
        broker: 'mqtt://alderaan.software-engineering.ie',
        port: 1883,
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

let db = null;
let mqttClient = null;
let batchCounter = 0;

// Multi-device status tracking
const devices = new Map();

function getOrCreateDevice(deviceId) {
    if (!devices.has(deviceId)) {
        devices.set(deviceId, {
            id: deviceId,
            connected: true,
            thresholds: { crash: 11.0, braking: 9.0, accel: 7.0, cornering: 8.0 },
            lastUpdate: Date.now()
        });
    }
    return devices.get(deviceId);
}

// ============== Database Setup ==============

function initDatabase() {
    console.log(`[SQLite] Opening database: ${config.sqlite.filename}`);
    db = new Database(config.sqlite.filename);

    db.exec(`
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL DEFAULT 'unknown',
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
            device_id TEXT NOT NULL DEFAULT 'unknown',
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

        CREATE INDEX IF NOT EXISTS idx_alerts_device ON alerts(device_id);
        CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(type);
        CREATE INDEX IF NOT EXISTS idx_alerts_created ON alerts(created_at);
        CREATE INDEX IF NOT EXISTS idx_readings_device ON sensor_readings(device_id);
        CREATE INDEX IF NOT EXISTS idx_readings_batch ON sensor_readings(batch_id);
        CREATE INDEX IF NOT EXISTS idx_readings_timestamp ON sensor_readings(calculated_timestamp);
    `);

    const maxBatch = db.prepare('SELECT MAX(batch_id) as max FROM sensor_readings').get();
    batchCounter = (maxBatch.max || 0) + 1;

    db.insertAlert = db.prepare(`
        INSERT INTO alerts (device_id, type, event, device_timestamp, accel_magnitude, accel_x, accel_y, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    `);

    db.insertReading = db.prepare(`
        INSERT INTO sensor_readings (device_id, batch_id, sample_index, batch_start_timestamp, sample_rate_hz, calculated_timestamp, x, y, z, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `);

    db.insertReadingsBatch = db.transaction((deviceId, batchId, batchStartTimestamp, sampleRateHz, samples, receivedAt) => {
        for (let i = 0; i < samples.length; i++) {
            const calculatedTimestamp = batchStartTimestamp + Math.floor(i * 1000 / sampleRateHz);
            const [x, y, z] = samples[i];
            db.insertReading.run(deviceId, batchId, i, batchStartTimestamp, sampleRateHz, calculatedTimestamp, x, y, z, receivedAt);
        }
    });

    console.log(`[SQLite] Database initialized (next batch_id: ${batchCounter})`);
}

// ============== MQTT Handlers ==============

function handleAlert(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();
    const deviceId = data.dev || 'unknown';

    getOrCreateDevice(deviceId);

    if (data.type === 'crash') {
        db.insertAlert.run(deviceId, 'crash', null, data.ts, data.mag, null, null, receivedAt);
        console.log(`[Alert] ${deviceId}: CRASH magnitude=${data.mag}`);
    } else if (data.type === 'warning') {
        db.insertAlert.run(deviceId, 'warning', data.event, data.ts, null, data.x, data.y, receivedAt);
        console.log(`[Alert] ${deviceId}: WARNING ${data.event}`);
    }
}

function handleTelemetry(message) {
    const data = JSON.parse(message.toString());
    const receivedAt = Date.now();
    const deviceId = data.dev || 'unknown';
    const batchId = batchCounter++;

    getOrCreateDevice(deviceId);
    db.insertReadingsBatch(deviceId, batchId, data.ts, data.rate, data.d, receivedAt);
    console.log(`[Telemetry] ${deviceId}: ${data.n} samples (batch_id: ${batchId})`);
}

function handleStatus(message) {
    const data = JSON.parse(message.toString());
    const deviceId = data.dev || 'unknown';

    const device = getOrCreateDevice(deviceId);
    device.connected = true;
    device.thresholds = {
        crash: data.crash,
        braking: data.braking,
        accel: data.accel,
        cornering: data.cornering
    };
    device.lastUpdate = Date.now();

    console.log(`[Status] ${deviceId}: crash=${data.crash} braking=${data.braking} accel=${data.accel} cornering=${data.cornering}`);
}

function connectMQTT() {
    console.log(`[MQTT] Connecting to ${config.mqtt.broker}...`);
    mqttClient = mqtt.connect(config.mqtt.broker, config.mqtt.options);

    mqttClient.on('connect', () => {
        console.log('[MQTT] Connected to broker');
        mqttClient.subscribe('driving/alerts', { qos: 1 });
        mqttClient.subscribe('driving/telemetry', { qos: 0 });
        mqttClient.subscribe('driving/status', { qos: 1 });
    });

    mqttClient.on('message', (topic, message) => {
        try {
            if (topic === 'driving/alerts') handleAlert(message);
            else if (topic === 'driving/telemetry') handleTelemetry(message);
            else if (topic === 'driving/status') handleStatus(message);
        } catch (error) {
            console.error(`[MQTT] Error:`, error.message);
        }
    });

    mqttClient.on('error', (err) => console.error('[MQTT] Error:', err.message));
    mqttClient.on('reconnect', () => console.log('[MQTT] Reconnecting...'));
}

// ============== REST API ==============

app.use(express.json());

// List all known devices
app.get('/api/devices', (req, res) => {
    const deviceList = Array.from(devices.values()).map(d => ({
        id: d.id,
        connected: d.connected,
        lastUpdate: d.lastUpdate
    }));
    res.json(deviceList);
});

// Get device status
app.get('/api/devices/:deviceId/status', (req, res) => {
    const device = devices.get(req.params.deviceId);
    if (!device) {
        return res.status(404).json({ error: 'Device not found' });
    }
    res.json(device);
});

// Get alerts (optionally filtered by device)
app.get('/api/alerts', (req, res) => {
    const limit = parseInt(req.query.limit) || 50;
    const deviceId = req.query.device;

    let query = 'SELECT * FROM alerts';
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' ORDER BY created_at DESC LIMIT ?';
    params.push(limit);

    const alerts = db.prepare(query).all(...params);
    res.json(alerts);
});

// Get alert summary (optionally filtered by device)
app.get('/api/alerts/summary', (req, res) => {
    const deviceId = req.query.device;

    let query = `SELECT type, event, COUNT(*) as count FROM alerts`;
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' GROUP BY type, event';

    const summary = db.prepare(query).all(...params);
    res.json(summary);
});

// Get stats (optionally filtered by device)
app.get('/api/stats', (req, res) => {
    const deviceId = req.query.device;

    let alertQuery = 'SELECT COUNT(*) as count FROM alerts';
    let crashQuery = "SELECT COUNT(*) as count FROM alerts WHERE type='crash'";
    let readingQuery = 'SELECT COUNT(*) as count FROM sensor_readings';
    let batchQuery = 'SELECT COUNT(DISTINCT batch_id) as count FROM sensor_readings';

    if (deviceId) {
        alertQuery += ' WHERE device_id = ?';
        crashQuery += ' AND device_id = ?';
        readingQuery += ' WHERE device_id = ?';
        batchQuery += ' WHERE device_id = ?';

        const alertCount = db.prepare(alertQuery).get(deviceId);
        const crashCount = db.prepare(crashQuery).get(deviceId);
        const readingCount = db.prepare(readingQuery).get(deviceId);
        const batchCount = db.prepare(batchQuery).get(deviceId);

        res.json({
            totalAlerts: alertCount.count,
            crashes: crashCount.count,
            warnings: alertCount.count - crashCount.count,
            totalReadings: readingCount.count,
            totalBatches: batchCount.count
        });
    } else {
        const alertCount = db.prepare(alertQuery).get();
        const crashCount = db.prepare(crashQuery).get();
        const readingCount = db.prepare(readingQuery).get();
        const batchCount = db.prepare(batchQuery).get();

        res.json({
            totalAlerts: alertCount.count,
            crashes: crashCount.count,
            warnings: alertCount.count - crashCount.count,
            totalReadings: readingCount.count,
            totalBatches: batchCount.count
        });
    }
});

// Get latest readings (optionally filtered by device)
app.get('/api/readings/latest', (req, res) => {
    const limit = parseInt(req.query.limit) || 500;
    const deviceId = req.query.device;

    let query = 'SELECT device_id, x, y, z, calculated_timestamp, batch_id, sample_index FROM sensor_readings';
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' ORDER BY batch_id DESC, sample_index DESC LIMIT ?';
    params.push(limit);

    const readings = db.prepare(query).all(...params);
    res.json(readings.reverse());
});

// Get batches (optionally filtered by device)
app.get('/api/batches', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const deviceId = req.query.device;

    let query = `
        SELECT device_id, batch_id, batch_start_timestamp, sample_rate_hz,
               COUNT(*) as sample_count, MIN(created_at) as created_at
        FROM sensor_readings
    `;
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' GROUP BY batch_id ORDER BY batch_id DESC LIMIT ?';
    params.push(limit);

    const batches = db.prepare(query).all(...params);
    res.json(batches);
});

// Get batch by ID
app.get('/api/batches/:batchId', (req, res) => {
    const readings = db.prepare(`
        SELECT sample_index, x, y, z, calculated_timestamp
        FROM sensor_readings
        WHERE batch_id = ?
        ORDER BY sample_index
    `).all(req.params.batchId);
    res.json(readings);
});

// Send command to specific device
app.post('/api/devices/:deviceId/threshold', (req, res) => {
    const { deviceId } = req.params;
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

    // Publish to device-specific command topic
    const topic = `driving/commands/${deviceId}`;
    mqttClient.publish(topic, command, { qos: 1 }, (err) => {
        if (err) {
            console.error('[MQTT] Failed to publish command:', err.message);
            return res.status(500).json({ error: 'Failed to send command' });
        }
        console.log(`[Config] ${deviceId}: ${type}=${value}G`);
        res.json({ success: true, deviceId, type, value });
    });
});

// Legacy endpoint (for backwards compatibility)
app.get('/api/device/status', (req, res) => {
    // Return first device or default
    const firstDevice = devices.values().next().value;
    if (firstDevice) {
        res.json(firstDevice);
    } else {
        res.json({
            connected: false,
            thresholds: { crash: 11.0, braking: 9.0, accel: 7.0, cornering: 8.0 },
            lastUpdate: null
        });
    }
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
    console.log('  Driving Safety Monitor - Multi-Device Dashboard');
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
