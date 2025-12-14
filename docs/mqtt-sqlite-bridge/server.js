/**
 * Driving Safety Monitor - Dashboard Server
 * Entry point for the MQTT-SQLite bridge and REST API
 */

const express = require('express');
const path = require('path');

const config = require('./src/config');
const db = require('./src/database');
const mqtt = require('./src/mqtt');

// Routes
const alertsRouter = require('./src/routes/alerts');
const devicesRouter = require('./src/routes/devices');
const readingsRouter = require('./src/routes/readings');
const statsRouter = require('./src/routes/stats');

const app = express();
app.use(express.json());

// API routes
app.use('/api/alerts', alertsRouter);
app.use('/api/devices', devicesRouter);
app.use('/api/readings', readingsRouter);
app.use('/api/batches', readingsRouter);
app.use('/api/stats', statsRouter);
app.use('/api/score', statsRouter);

// Legacy endpoint
app.get('/api/device/status', (req, res) => {
    const devices = require('./src/devices');
    const firstDevice = devices.getFirst();
    if (firstDevice) {
        res.json(firstDevice);
    } else {
        res.json({
            connected: false,
            thresholds: { ...config.defaults.thresholds },
            lastUpdate: null
        });
    }
});

// Dashboard
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'dashboard.html'));
});

// Shutdown handler
function shutdown() {
    console.log('\n[System] Shutting down...');
    mqtt.disconnect();
    db.close();
    process.exit(0);
}

// Main
function main() {
    console.log('='.repeat(50));
    console.log('  Driving Safety Monitor - Dashboard Server');
    console.log('='.repeat(50));

    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);

    db.init();
    mqtt.connect();

    app.listen(config.port, () => {
        console.log(`[HTTP] Dashboard running at http://localhost:${config.port}`);
    });
}

main();
