const mqtt = require('mqtt');
const config = require('./config');
const db = require('./database');
const devices = require('./devices');

let client = null;

function connect() {
    console.log(`[MQTT] Connecting to ${config.mqtt.broker}...`);
    client = mqtt.connect(config.mqtt.broker, config.mqtt.options);

    client.on('connect', onConnect);
    client.on('message', onMessage);
    client.on('error', (err) => console.error('[MQTT] Error:', err.message));
    client.on('reconnect', () => console.log('[MQTT] Reconnecting...'));

    return client;
}

function disconnect() {
    if (client) client.end();
}

function onConnect() {
    console.log('[MQTT] Connected to broker');
    client.subscribe(config.mqtt.topics.alerts, { qos: config.mqtt.qos.alerts });
    client.subscribe(config.mqtt.topics.telemetry, { qos: config.mqtt.qos.telemetry });
    client.subscribe(config.mqtt.topics.status, { qos: config.mqtt.qos.status });
}

function onMessage(topic, message) {
    try {
        const data = JSON.parse(message.toString());

        switch (topic) {
            case config.mqtt.topics.alerts:
                handleAlert(data);
                break;
            case config.mqtt.topics.telemetry:
                handleTelemetry(data);
                break;
            case config.mqtt.topics.status:
                handleStatus(data);
                break;
        }
    } catch (error) {
        console.error('[MQTT] Error:', error.message);
    }
}

function handleAlert(data) {
    const deviceId = data.dev || 'unknown';
    devices.getOrCreate(deviceId);

    if (data.type === 'crash') {
        db.insertAlert(deviceId, 'crash', null, data.ts, data.mag, null, null);
        console.log(`[Alert] ${deviceId}: CRASH magnitude=${data.mag}`);
    } else if (data.type === 'warning') {
        db.insertAlert(deviceId, 'warning', data.event, data.ts, null, data.x, data.y);
        console.log(`[Alert] ${deviceId}: WARNING ${data.event}`);
    }
}

function handleTelemetry(data) {
    const deviceId = data.dev || 'unknown';
    const batchId = db.getNextBatchId();

    devices.getOrCreate(deviceId);
    db.insertReadingsBatch(deviceId, batchId, data.ts, data.rate, data.d);
    console.log(`[Telemetry] ${deviceId}: ${data.n} samples (batch_id: ${batchId})`);
}

function handleStatus(data) {
    const deviceId = data.dev || 'unknown';
    const thresholds = {
        crash: data.crash,
        braking: data.braking,
        accel: data.accel,
        cornering: data.cornering
    };

    devices.updateStatus(deviceId, thresholds);
    console.log(`[Status] ${deviceId}: crash=${data.crash} braking=${data.braking} accel=${data.accel} cornering=${data.cornering}`);
}

function publishCommand(deviceId, command) {
    return new Promise((resolve, reject) => {
        const topic = `${config.mqtt.topics.commands}/${deviceId}`;
        const payload = JSON.stringify(command);

        client.publish(topic, payload, { qos: config.mqtt.qos.commands }, (err) => {
            if (err) {
                console.error('[MQTT] Failed to publish command:', err.message);
                reject(err);
            } else {
                console.log(`[Command] ${deviceId}: ${JSON.stringify(command)}`);
                resolve();
            }
        });
    });
}

module.exports = {
    connect,
    disconnect,
    publishCommand
};
