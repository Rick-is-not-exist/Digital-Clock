const express = require('express');
const { pool } = require('../config/database');
const { publishCommand } = require('../services/mqtt');
const auth = require('../middleware/auth');

const router = express.Router();

router.use(auth);

router.get('/', async (req, res) => {
  try {
    const result = await pool.query(
      `SELECT d.id, d.device_uid, d.name,
              CASE WHEN d.last_seen > NOW() - INTERVAL '2 minutes' THEN true ELSE false END as online,
              d.last_seen, d.created_at,
              ds.text1, ds.anim, ds.speed, ds.clock_duration, ds.text_duration,
              ds.display_mode, ds.brightness, ds.auto_dimming, COALESCE(ds.power, true) as power
       FROM devices d
       LEFT JOIN device_settings ds ON ds.device_id = d.id
       WHERE d.user_id = $1
       ORDER BY d.created_at DESC`,
      [req.user.id]
    );
    res.json(result.rows);
  } catch (err) {
    console.error('[DEVICES] List error:', err.message);
    res.status(500).json({ error: 'Server error' });
  }
});

router.post('/', async (req, res) => {
  try {
    const { device_uid, name } = req.body;
    if (!device_uid) {
      return res.status(400).json({ error: 'device_uid required' });
    }

    const existing = await pool.query('SELECT id FROM devices WHERE device_uid = $1', [device_uid]);
    if (existing.rows.length > 0) {
      return res.status(409).json({ error: 'Device already registered' });
    }

    const topic = `p10/${device_uid}/commands`;
    const result = await pool.query(
      'INSERT INTO devices (device_uid, user_id, name, mqtt_topic) VALUES ($1, $2, $3, $4) RETURNING *',
      [device_uid, req.user.id, name || 'P10 Panel', topic]
    );

    const device = result.rows[0];

    await pool.query(
      'INSERT INTO device_settings (device_id) VALUES ($1)',
      [device.id]
    );

    res.status(201).json(device);
  } catch (err) {
    console.error('[DEVICES] Create error:', err.message);
    res.status(500).json({ error: 'Server error' });
  }
});

router.get('/:id', async (req, res) => {
  try {
    const result = await pool.query(
      `SELECT d.id, d.device_uid, d.name,
              CASE WHEN d.last_seen > NOW() - INTERVAL '2 minutes' THEN true ELSE false END as online,
              d.last_seen, d.created_at,
              ds.text1, ds.anim, ds.speed, ds.clock_duration, ds.text_duration,
              ds.display_mode, ds.brightness, ds.auto_dimming, COALESCE(ds.power, true) as power
       FROM devices d
       LEFT JOIN device_settings ds ON ds.device_id = d.id
       WHERE d.id = $1 AND d.user_id = $2`,
      [req.params.id, req.user.id]
    );
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Device not found' });
    }
    res.json(result.rows[0]);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

router.put('/:id/settings', async (req, res) => {
  try {
    const { id } = req.params;
    const deviceCheck = await pool.query(
      'SELECT device_uid FROM devices WHERE id = $1 AND user_id = $2',
      [id, req.user.id]
    );
    if (deviceCheck.rows.length === 0) {
      return res.status(404).json({ error: 'Device not found' });
    }

    const { text1, anim, speed, clock_duration, text_duration, display_mode, brightness, auto_dimming, power } = req.body;

    const result = await pool.query(
      `INSERT INTO device_settings (device_id, text1, anim, speed, clock_duration, text_duration, display_mode, brightness, auto_dimming, power, updated_at)
       VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, NOW())
       ON CONFLICT (device_id) DO UPDATE SET
         text1 = COALESCE($2, device_settings.text1),
         anim = COALESCE($3, device_settings.anim),
         speed = COALESCE($4, device_settings.speed),
         clock_duration = COALESCE($5, device_settings.clock_duration),
         text_duration = COALESCE($6, device_settings.text_duration),
         display_mode = COALESCE($7, device_settings.display_mode),
         brightness = COALESCE($8, device_settings.brightness),
         auto_dimming = COALESCE($9, device_settings.auto_dimming),
         power = COALESCE($10, device_settings.power),
         updated_at = NOW()
       RETURNING *`,
      [id, text1, anim, speed, clock_duration, text_duration, display_mode, brightness, auto_dimming, power]
    );

    const deviceUID = deviceCheck.rows[0].device_uid;
    const settings = result.rows[0];

    const mqttPayload = {
      text1: settings.text1,
      anim: settings.anim,
      speed_ms: Math.max(15, 120 - (settings.speed * 10)),
      clock_duration: settings.clock_duration,
      text_duration: settings.text_duration,
      mode: settings.display_mode,
      brightness_pwm: Math.round((settings.brightness / 100) * 255),
      power: settings.power !== false,
      format_24h: true,
      show_seconds: true
    };

    publishCommand(deviceUID, mqttPayload);

    await pool.query(
      'INSERT INTO command_logs (device_id, command_type, payload) VALUES ($1, $2, $3)',
      [id, 'settings_update', JSON.stringify(mqttPayload)]
    );

    res.json(settings);
  } catch (err) {
    console.error('[DEVICES] Settings update error:', err.message);
    res.status(500).json({ error: 'Server error' });
  }
});

router.post('/:id/sync-time', async (req, res) => {
  try {
    const { id } = req.params;
    const deviceCheck = await pool.query(
      'SELECT device_uid FROM devices WHERE id = $1 AND user_id = $2',
      [id, req.user.id]
    );
    if (deviceCheck.rows.length === 0) {
      return res.status(404).json({ error: 'Device not found' });
    }

    const deviceUID = deviceCheck.rows[0].device_uid;
    publishCommand(deviceUID, { time_sync: "ok" });

    res.json({ status: 'ok', message: 'Time synced via NTP' });
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

router.get('/:id/status', async (req, res) => {
  try {
    const result = await pool.query(
      'SELECT online, last_seen FROM devices WHERE id = $1 AND user_id = $2',
      [req.params.id, req.user.id]
    );
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Device not found' });
    }
    res.json(result.rows[0]);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    const result = await pool.query(
      'DELETE FROM devices WHERE id = $1 AND user_id = $2 RETURNING id',
      [req.params.id, req.user.id]
    );
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Device not found' });
    }
    res.json({ status: 'ok', message: 'Device deleted' });
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

module.exports = router;
