const express = require('express');
const db = require('../database');

const router = express.Router();

// Get latest readings
router.get('/latest', (req, res) => {
    const limit = parseInt(req.query.limit) || 500;
    const deviceId = req.query.device;
    res.json(db.getLatestReadings(deviceId, limit));
});

// Get batches
router.get('/batches', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const deviceId = req.query.device;
    res.json(db.getBatches(deviceId, limit));
});

// Get batch by ID
router.get('/batches/:batchId', (req, res) => {
    res.json(db.getBatchById(req.params.batchId));
});

module.exports = router;
