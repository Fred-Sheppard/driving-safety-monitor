const config = require('./config');

// In-memory device status tracking
const devices = new Map();

function getOrCreate(deviceId) {
    if (!devices.has(deviceId)) {
        devices.set(deviceId, {
            id: deviceId,
            connected: true,
            thresholds: { ...config.defaults.thresholds },
            lastUpdate: Date.now()
        });
    }
    return devices.get(deviceId);
}

function get(deviceId) {
    return devices.get(deviceId);
}

function getAll() {
    return Array.from(devices.values());
}

function updateStatus(deviceId, thresholds) {
    const device = getOrCreate(deviceId);
    device.connected = true;
    device.thresholds = thresholds;
    device.lastUpdate = Date.now();
    return device;
}

function getFirst() {
    return devices.values().next().value || null;
}

module.exports = {
    getOrCreate,
    get,
    getAll,
    updateStatus,
    getFirst
};
