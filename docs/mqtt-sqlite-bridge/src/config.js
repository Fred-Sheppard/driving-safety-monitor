const path = require('path');

module.exports = {
    port: process.env.PORT || 3001,

    mqtt: {
        broker: 'mqtt://alderaan.software-engineering.ie',
        topics: {
            alerts: 'driving/alerts',
            telemetry: 'driving/telemetry',
            status: 'driving/status',
            commands: 'driving/commands'
        },
        qos: {
            alerts: 1,
            telemetry: 0,
            status: 1,
            commands: 1
        },
        options: {
            clientId: `driving-monitor-bridge-${Math.random().toString(16).slice(2, 8)}`,
            clean: true,
            connectTimeout: 4000,
            reconnectPeriod: 1000
        }
    },

    database: {
        filename: path.join(__dirname, '..', 'driving_monitor.db')
    },

    defaults: {
        thresholds: {
            crash: 3.0,
            braking: 2.0,
            accel: 1.5,
            cornering: 2.0
        }
    }
};
