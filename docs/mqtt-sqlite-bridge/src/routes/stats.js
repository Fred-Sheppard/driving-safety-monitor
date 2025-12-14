const express = require('express');
const db = require('../database');

const router = express.Router();

// Get stats
router.get('/', (req, res) => {
    const deviceId = req.query.device;
    res.json(db.getStats(deviceId));
});

// Reset score (delete alerts)
router.post('/reset', (req, res) => {
    const deviceId = req.query.device;
    db.deleteAlerts(deviceId);

    if (deviceId) {
        console.log(`[Reset] Cleared alerts for device ${deviceId}`);
    } else {
        console.log('[Reset] Cleared all alerts');
    }

    res.json({ success: true });
});

module.exports = router;
