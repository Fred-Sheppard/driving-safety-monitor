const express = require('express');
const db = require('../database');

const router = express.Router();

// Get alerts (optionally filtered by device)
router.get('/', (req, res) => {
    const limit = parseInt(req.query.limit) || 50;
    const deviceId = req.query.device;
    res.json(db.getAlerts(deviceId, limit));
});

// Get alert summary
router.get('/summary', (req, res) => {
    const deviceId = req.query.device;
    res.json(db.getAlertSummary(deviceId));
});

// Get event history (hourly buckets)
router.get('/history', (req, res) => {
    const deviceId = req.query.device;
    const hours = parseInt(req.query.hours) || 24;
    res.json(db.getAlertHistory(deviceId, hours));
});

module.exports = router;
