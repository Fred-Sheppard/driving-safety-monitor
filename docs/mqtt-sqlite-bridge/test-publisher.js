/**
 * Test Publisher - Simulates embedded device messages
 * Use this to test the MQTT to SQLite bridge
 */

const mqtt = require('mqtt');

const config = {
    broker: 'mqtt://alderaan.software-engineering.ie',
    options: {
        clientId: `blackbox-test-${Math.random().toString(16).slice(2, 8)}`,
    }
};

const client = mqtt.connect(config.broker, config.options);

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    
    // Simulate telemetry data every 2 seconds
    setInterval(() => {
        const telemetry = {
            deviceId: 'blackbox-001',
            timestamp: Date.now(),
            acceleration: {
                x: (Math.random() * 2 - 1).toFixed(3),
                y: (Math.random() * 2 - 1).toFixed(3),
                z: (Math.random() * 0.5 + 9.5).toFixed(3)
            },
            speed: Math.floor(Math.random() * 120),
            braking: Math.random() > 0.8,
            harshAcceleration: Math.random() > 0.9,
            safetyScore: Math.floor(Math.random() * 30 + 70)
        };
        
        client.publish('blackbox/telemetry', JSON.stringify(telemetry));
        console.log('Published telemetry:', telemetry);
    }, 2000);
    
    // Simulate braking events occasionally
    setInterval(() => {
        if (Math.random() > 0.7) {
            const brakingEvent = {
                deviceId: 'blackbox-001',
                timestamp: Date.now(),
                type: 'harsh_braking',
                deceleration: (Math.random() * 5 + 3).toFixed(2),
                duration_ms: Math.floor(Math.random() * 2000 + 500)
            };
            
            client.publish('blackbox/braking', JSON.stringify(brakingEvent));
            console.log('Published braking event:', brakingEvent);
        }
    }, 5000);
});

client.on('error', (error) => {
    console.error('MQTT Error:', error.message);
});

console.log('Test publisher starting...');
