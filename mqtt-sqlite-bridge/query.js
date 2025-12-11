/**
 * Query utility for the driving monitor SQLite database
 * Usage: node query.js [command]
 *
 * Commands:
 *   alerts     - Show recent alerts (crashes and warnings)
 *   crashes    - Show crash events only
 *   warnings   - Show warning events only
 *   batches    - Show recent telemetry batches
 *   readings   - Show recent sensor readings
 *   stats      - Show database statistics
 */

const Database = require('better-sqlite3');
const path = require('path');

const dbPath = path.join(__dirname, 'driving_monitor.db');

let db;
try {
    db = new Database(dbPath, { readonly: true });
} catch (err) {
    console.error(`Error opening database: ${err.message}`);
    console.log('Make sure the bridge has run at least once to create the database.');
    process.exit(1);
}

const command = process.argv[2] || 'stats';

console.log('='.repeat(50));
console.log('  Driving Safety Monitor - Database Query');
console.log('='.repeat(50));

switch (command) {
    case 'alerts':
        console.log('\nRecent alerts (last 20):\n');
        const alerts = db.prepare(`
            SELECT id, type, event, device_timestamp, accel_magnitude, accel_y, created_at
            FROM alerts
            ORDER BY created_at DESC
            LIMIT 20
        `).all();
        if (alerts.length === 0) {
            console.log('  No alerts recorded yet.');
        } else {
            alerts.forEach(row => {
                if (row.type === 'crash') {
                    console.log(`[${row.created_at}] CRASH - magnitude: ${row.accel_magnitude}`);
                } else {
                    console.log(`[${row.created_at}] WARNING: ${row.event} - accel_y: ${row.accel_y}`);
                }
            });
        }
        break;

    case 'crashes':
        console.log('\nCrash events:\n');
        const crashes = db.prepare(`
            SELECT id, device_timestamp, accel_magnitude, created_at
            FROM alerts
            WHERE type = 'crash'
            ORDER BY created_at DESC
            LIMIT 20
        `).all();
        if (crashes.length === 0) {
            console.log('  No crash events recorded.');
        } else {
            crashes.forEach(row => {
                console.log(`[${row.created_at}] Magnitude: ${row.accel_magnitude} (device_ts: ${row.device_timestamp})`);
            });
        }
        break;

    case 'warnings':
        console.log('\nWarning events:\n');
        const warnings = db.prepare(`
            SELECT id, event, device_timestamp, accel_y, created_at
            FROM alerts
            WHERE type = 'warning'
            ORDER BY created_at DESC
            LIMIT 20
        `).all();
        if (warnings.length === 0) {
            console.log('  No warning events recorded.');
        } else {
            warnings.forEach(row => {
                console.log(`[${row.created_at}] ${row.event} - accel_y: ${row.accel_y}`);
            });
        }
        break;

    case 'batches':
        console.log('\nRecent telemetry batches (last 10):\n');
        const batches = db.prepare(`
            SELECT id, batch_start_timestamp, sample_rate_hz, sample_count, created_at
            FROM telemetry_batches
            ORDER BY created_at DESC
            LIMIT 10
        `).all();
        if (batches.length === 0) {
            console.log('  No telemetry batches recorded yet.');
        } else {
            batches.forEach(row => {
                console.log(`[${row.created_at}] Batch #${row.id}: ${row.sample_count} samples @ ${row.sample_rate_hz}Hz`);
            });
        }
        break;

    case 'readings':
        console.log('\nRecent sensor readings (last 20):\n');
        const readings = db.prepare(`
            SELECT sr.id, sr.batch_id, sr.sample_index, sr.x, sr.y, sr.z, sr.calculated_timestamp
            FROM sensor_readings sr
            ORDER BY sr.id DESC
            LIMIT 20
        `).all();
        if (readings.length === 0) {
            console.log('  No sensor readings recorded yet.');
        } else {
            console.log('  batch_id | index |    x     |    y     |    z');
            console.log('  ' + '-'.repeat(50));
            readings.forEach(row => {
                console.log(`     ${row.batch_id.toString().padStart(4)}  | ${row.sample_index.toString().padStart(5)} | ${row.x.toFixed(4).padStart(8)} | ${row.y.toFixed(4).padStart(8)} | ${row.z.toFixed(4).padStart(8)}`);
            });
        }
        break;

    case 'stats':
        console.log('\nDatabase statistics:\n');

        const alertCount = db.prepare('SELECT COUNT(*) as count FROM alerts').get();
        const crashCount = db.prepare("SELECT COUNT(*) as count FROM alerts WHERE type = 'crash'").get();
        const warningCount = db.prepare("SELECT COUNT(*) as count FROM alerts WHERE type = 'warning'").get();
        const batchCount = db.prepare('SELECT COUNT(*) as count FROM telemetry_batches').get();
        const readingCount = db.prepare('SELECT COUNT(*) as count FROM sensor_readings').get();

        console.log('  Alerts:');
        console.log(`    Total:    ${alertCount.count}`);
        console.log(`    Crashes:  ${crashCount.count}`);
        console.log(`    Warnings: ${warningCount.count}`);
        console.log('');
        console.log('  Telemetry:');
        console.log(`    Batches:  ${batchCount.count}`);
        console.log(`    Readings: ${readingCount.count}`);

        if (batchCount.count > 0) {
            const latestBatch = db.prepare(`
                SELECT created_at FROM telemetry_batches ORDER BY created_at DESC LIMIT 1
            `).get();
            console.log(`    Latest:   ${latestBatch.created_at}`);
        }
        break;

    default:
        console.log('\nUnknown command. Available commands:');
        console.log('  alerts   - Show recent alerts (crashes and warnings)');
        console.log('  crashes  - Show crash events only');
        console.log('  warnings - Show warning events only');
        console.log('  batches  - Show recent telemetry batches');
        console.log('  readings - Show recent sensor readings');
        console.log('  stats    - Show database statistics');
}

db.close();
