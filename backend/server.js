require('dotenv').config();
const express = require('express');
const cors = require('cors');
const { initDB } = require('./config/database');
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
  } catch (err) {
    console.error('[SERVER] Failed to start:', err.message);
    process.exit(1);
  }
}

start();
