const express = require('express');
const devices = require('../devices');
const mqtt = require('../mqtt');
const config = require('../config');

const router = express.Router();

// List all devices
router.get('/', (req, res) => {
    const deviceList = devices.getAll().map(d => ({
        id: d.id,
        connected: d.connected,
        lastUpdate: d.lastUpdate
    }));
    res.json(deviceList);
});

// Get device status
router.get('/:deviceId/status', (req, res) => {
    const device = devices.get(req.params.deviceId);
    if (!device) {
        return res.status(404).json({ error: 'Device not found' });
    }
    res.json(device);
});

// Send threshold command to device
router.post('/:deviceId/threshold', async (req, res) => {
    const { deviceId } = req.params;
    const { type, value } = req.body;

    const validTypes = ['crash', 'braking', 'accel', 'cornering'];
    if (!validTypes.includes(type)) {
        return res.status(400).json({ error: 'Invalid threshold type' });
    }

    if (typeof value !== 'number' || value < 0 || value > 50) {
        return res.status(400).json({ error: 'Value must be a number between 0 and 50' });
    }

    try {
        await mqtt.publishCommand(deviceId, {
            cmd: 'set_threshold',
            type,
            value
        });
        console.log(`[Config] ${deviceId}: ${type}=${value}G`);
        res.json({ success: true, deviceId, type, value });
    } catch (err) {
        res.status(500).json({ error: 'Failed to send command' });
    }
});

// Legacy endpoint
router.get('/status', (req, res) => {
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

module.exports = router;
