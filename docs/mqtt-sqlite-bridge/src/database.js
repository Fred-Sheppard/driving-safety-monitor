const Database = require('better-sqlite3');
const config = require('./config');

let db = null;
let batchCounter = 0;

function init() {
    console.log(`[SQLite] Opening database: ${config.database.filename}`);
    db = new Database(config.database.filename);

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

    console.log(`[SQLite] Database initialized (next batch_id: ${batchCounter})`);
}

function close() {
    if (db) db.close();
}

function getNextBatchId() {
    return batchCounter++;
}

// Alert operations
function insertAlert(deviceId, type, event, timestamp, magnitude, accelX, accelY) {
    const receivedAt = Date.now();
    db.prepare(`
        INSERT INTO alerts (device_id, type, event, device_timestamp, accel_magnitude, accel_x, accel_y, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    `).run(deviceId, type, event, timestamp, magnitude, accelX, accelY, receivedAt);
}

function getAlerts(deviceId, limit = 50) {
    let query = 'SELECT * FROM alerts';
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' ORDER BY created_at DESC LIMIT ?';
    params.push(limit);

    return db.prepare(query).all(...params);
}

function getAlertSummary(deviceId) {
    let query = 'SELECT type, event, COUNT(*) as count FROM alerts';
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' GROUP BY type, event';

    return db.prepare(query).all(...params);
}

function getAlertHistory(deviceId, hours = 24) {
    const cutoff = Date.now() - (hours * 60 * 60 * 1000);

    let query = `
        SELECT
            strftime('%Y-%m-%d %H:00', datetime(received_at/1000, 'unixepoch', 'localtime')) as hour,
            type,
            COUNT(*) as count
        FROM alerts
        WHERE received_at >= ?
    `;
    const params = [cutoff];

    if (deviceId) {
        query += ' AND device_id = ?';
        params.push(deviceId);
    }
    query += ' GROUP BY hour, type ORDER BY hour ASC';

    return db.prepare(query).all(...params);
}

function deleteAlerts(deviceId) {
    if (deviceId) {
        db.prepare('DELETE FROM alerts WHERE device_id = ?').run(deviceId);
    } else {
        db.prepare('DELETE FROM alerts').run();
    }
}

// Sensor reading operations
function insertReadingsBatch(deviceId, batchId, batchStartTimestamp, sampleRateHz, samples) {
    const receivedAt = Date.now();
    const insertReading = db.prepare(`
        INSERT INTO sensor_readings (device_id, batch_id, sample_index, batch_start_timestamp, sample_rate_hz, calculated_timestamp, x, y, z, received_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `);

    const transaction = db.transaction(() => {
        for (let i = 0; i < samples.length; i++) {
            const calculatedTimestamp = batchStartTimestamp + Math.floor(i * 1000 / sampleRateHz);
            const [x, y, z] = samples[i];
            insertReading.run(deviceId, batchId, i, batchStartTimestamp, sampleRateHz, calculatedTimestamp, x, y, z, receivedAt);
        }
    });

    transaction();
}

function getLatestReadings(deviceId, limit = 500) {
    let query = 'SELECT device_id, x, y, z, calculated_timestamp, batch_id, sample_index FROM sensor_readings';
    const params = [];

    if (deviceId) {
        query += ' WHERE device_id = ?';
        params.push(deviceId);
    }
    query += ' ORDER BY batch_id DESC, sample_index DESC LIMIT ?';
    params.push(limit);

    return db.prepare(query).all(...params).reverse();
}

function getBatches(deviceId, limit = 10) {
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

    return db.prepare(query).all(...params);
}

function getBatchById(batchId) {
    return db.prepare(`
        SELECT sample_index, x, y, z, calculated_timestamp
        FROM sensor_readings
        WHERE batch_id = ?
        ORDER BY sample_index
    `).all(batchId);
}

// Stats
function getStats(deviceId) {
    const whereDevice = deviceId ? ' WHERE device_id = ?' : '';
    const andDevice = deviceId ? ' AND device_id = ?' : '';
    const params = deviceId ? [deviceId] : [];

    const alertCount = db.prepare(`SELECT COUNT(*) as count FROM alerts${whereDevice}`).get(...params);
    const crashCount = db.prepare(`SELECT COUNT(*) as count FROM alerts WHERE type='crash'${andDevice}`).get(...params);
    const readingCount = db.prepare(`SELECT COUNT(*) as count FROM sensor_readings${whereDevice}`).get(...params);
    const batchCount = db.prepare(`SELECT COUNT(DISTINCT batch_id) as count FROM sensor_readings${whereDevice}`).get(...params);

    return {
        totalAlerts: alertCount.count,
        crashes: crashCount.count,
        warnings: alertCount.count - crashCount.count,
        totalReadings: readingCount.count,
        totalBatches: batchCount.count
    };
}

module.exports = {
    init,
    close,
    getNextBatchId,
    insertAlert,
    getAlerts,
    getAlertSummary,
    getAlertHistory,
    deleteAlerts,
    insertReadingsBatch,
    getLatestReadings,
    getBatches,
    getBatchById,
    getStats
};
