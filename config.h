#ifndef CONFIG_H
#define CONFIG_H

// === AP Mode (Captive Portal) ===
#define AP_PASS       ""  // Open network, no password

// === EEPROM Layout ===
#define EEPROM_SIZE   98
#define EEPROM_MAGIC  0xAB  // Magic byte to detect first boot
#define EEPROM_ADDR   0
#define EEPROM_SSID_ADDR  1
#define EEPROM_PASS_ADDR  33
#define EEPROM_SSID_LEN  32
#define EEPROM_PASS_LEN  64

// === MQTT (HiveMQ Cloud) ===
#define MQTT_HOST     "d34d0f0f7c86418d8c0f451a884ff4e7.s1.eu.hivemq.cloud"
#define MQTT_PORT     8883
#define MQTT_USER     "YOUR_MQTT_USER"
#define MQTT_PASS     "YOUR_MQTT_PASS"

// === Timing ===
#define WIFI_TIMEOUT  20000
#define STATUS_INTERVAL    30000
#define RECONNECT_INTERVAL 5000
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     8
#define NTP_DAYLIGHT       0
#define NTP_WAIT_TIME      15000

#endif
