const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
  ssl: process.env.NODE_ENV === 'production' ? { rejectUnauthorized: false } : false
});

async function initDB() {
  const client = await pool.connect();
  try {
    await client.query(`
      CREATE TABLE IF NOT EXISTS users (
        id SERIAL PRIMARY KEY,
        email VARCHAR(255) UNIQUE NOT NULL,
        password_hash VARCHAR(255) NOT NULL,
        name VARCHAR(100),
        created_at TIMESTAMP DEFAULT NOW()
      );

      CREATE TABLE IF NOT EXISTS devices (
        id SERIAL PRIMARY KEY,
        device_uid VARCHAR(64) UNIQUE NOT NULL,
        user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
        name VARCHAR(100) DEFAULT 'P10 Panel',
        mqtt_topic VARCHAR(255) NOT NULL,
        online BOOLEAN DEFAULT false,
        last_seen TIMESTAMP,
        local_host VARCHAR(255),
        created_at TIMESTAMP DEFAULT NOW()
      );

      CREATE TABLE IF NOT EXISTS device_settings (
        id SERIAL PRIMARY KEY,
        device_id INTEGER REFERENCES devices(id) ON DELETE CASCADE UNIQUE,
        text1 VARCHAR(150) DEFAULT 'SELAMAT DATANG',
        anim VARCHAR(20) DEFAULT 'scroll_left',
        speed INTEGER DEFAULT 5,
        clock_duration INTEGER DEFAULT 10,
        text_duration INTEGER DEFAULT 15,
        display_mode VARCHAR(20) DEFAULT 'cycle',
        brightness INTEGER DEFAULT 20,
        auto_dimming BOOLEAN DEFAULT false,
        power BOOLEAN DEFAULT true,
        updated_at TIMESTAMP DEFAULT NOW()
      );

      CREATE TABLE IF NOT EXISTS command_logs (
        id SERIAL PRIMARY KEY,
        device_id INTEGER REFERENCES devices(id) ON DELETE CASCADE,
        command_type VARCHAR(50),
        payload JSONB,
        sent_at TIMESTAMP DEFAULT NOW()
      );
    `);
    await client.query(`
      ALTER TABLE device_settings ADD COLUMN IF NOT EXISTS power BOOLEAN DEFAULT true
    `);
    await client.query(`
      ALTER TABLE devices ADD COLUMN IF NOT EXISTS local_host VARCHAR(255)
    `);
    console.log('[DB] Tables initialized');
  } finally {
    client.release();
  }
}

module.exports = { pool, initDB };
