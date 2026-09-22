require('dotenv').config();
const express = require('express');
const cors = require('cors');
const { initDB, pool } = require('./config/database');
const { connectMQTT } = require('./services/mqtt');
const authRoutes = require('./routes/auth');
const deviceRoutes = require('./routes/devices');

const app = express();

app.use(cors({
  origin: process.env.CORS_ORIGIN || '*',
  methods: ['GET', 'POST', 'PUT', 'DELETE'],
  allowedHeaders: ['Content-Type', 'Authorization']
}));

app.use(express.json());

app.get('/', (req, res) => {
  res.json({ name: 'P10 IoT Backend', version: '1.0.0', status: 'running' });
});

app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

app.use('/api/auth', authRoutes);
app.use('/api/devices', deviceRoutes);

const PORT = process.env.PORT || 3000;

async function start() {
  try {
    await initDB();
    console.log('[SERVER] Database initialized');

    connectMQTT();
    console.log('[SERVER] MQTT connecting...');

    app.listen(PORT, '0.0.0.0', () => {
      console.log(`[SERVER] Running on port ${PORT}`);
    });

    // Cleanup: mark devices offline if last_seen > 2 minutes
    setInterval(async () => {
      try {
        const result = await pool.query(
          `UPDATE devices SET online = false
           WHERE online = true AND last_seen < NOW() - INTERVAL '2 minutes'`
        );
        if (result.rowCount > 0) {
          console.log(`[CLEANUP] Marked ${result.rowCount} device(s) offline`);
        }
      } catch (e) {
        console.error('[CLEANUP] Error:', e.message);
      }
    }, 60000); // Check every 60 seconds
  } catch (err) {
    console.error('[SERVER] Failed to start:', err.message);
    process.exit(1);
  }
}

start();
