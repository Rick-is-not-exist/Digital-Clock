const mqtt = require('mqtt');
const { pool } = require('../config/database');

let client = null;

function connectMQTT() {
  const host = process.env.MQTT_HOST;
  const port = process.env.MQTT_PORT || 8883;
  const username = process.env.MQTT_USER;
  const password = process.env.MQTT_PASS;

  const url = `mqtts://${host}:${port}`;

  client = mqtt.connect(url, {
    username,
    password,
    clientId: `p10_backend_${Date.now()}`,
    protocolVersion: 4,
    clean: true,
    reconnectPeriod: 5000
  });

  client.on('connect', () => {
    console.log('[MQTT] Connected to HiveMQ Cloud');
    client.subscribe('p10/+/status', { qos: 1 });
  });

  client.on('message', (topic, message) => {
    try {
      const parts = topic.split('/');
      const deviceUID = parts[1];
      const payload = JSON.parse(message.toString());
      console.log(`[MQTT] Status from ${deviceUID}: ${JSON.stringify(payload)}`);
      handleStatus(deviceUID, payload);
    } catch (e) {
      console.error('[MQTT] Parse error:', e.message);
    }
  });

  client.on('error', (err) => {
    console.error('[MQTT] Error:', err.message);
  });

  client.on('reconnect', () => {
    console.log('[MQTT] Reconnecting...');
  });
}

async function handleStatus(deviceUID, payload) {
  try {
    const result = await pool.query(
      'UPDATE devices SET online = $1, last_seen = NOW() WHERE device_uid = $2',
      [payload.online === true, deviceUID]
    );
    console.log(`[MQTT] DB update: ${result.rowCount} row(s) for UID=${deviceUID}, online=${payload.online}`);
  } catch (e) {
    console.error('[MQTT] Status update error:', e.message);
  }
}

function publishCommand(deviceUID, payload) {
  if (!client || !client.connected) {
    console.error('[MQTT] Not connected');
    return false;
  }
  const topic = `p10/${deviceUID}/commands`;
  client.publish(topic, JSON.stringify(payload), { qos: 1 }, (err) => {
    if (err) {
      console.error('[MQTT] Publish error:', err.message);
    } else {
      console.log(`[MQTT] Published to ${topic}`);
    }
  });
  return true;
}

module.exports = { connectMQTT, publishCommand };
