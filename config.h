#ifndef CONFIG_H
#define CONFIG_H

// === WiFi Station Mode (connects to your home WiFi) ===
#define WIFI_SSID     "Rumah-2.4G"
#define WIFI_PASS     "Broklyn1985"
#define WIFI_TIMEOUT  20000

// === MQTT (HiveMQ Cloud) ===
#define MQTT_HOST     "d34d0f0f7c86418d8c0f451a884ff4e7.s1.eu.hivemq.cloud"
#define MQTT_PORT     8883
#define MQTT_USER     "JARVIS"
#define MQTT_PASS     "iloveyou3000"

// === Timing ===
#define STATUS_INTERVAL    30000
#define RECONNECT_INTERVAL 5000
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     8
#define NTP_DAYLIGHT       0
#define NTP_WAIT_TIME      15000

#endif
