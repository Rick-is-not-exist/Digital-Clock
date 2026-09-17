#ifndef CONFIG_H
#define CONFIG_H

// === WiFi Station Mode (connects to your home WiFi) ===
#define WIFI_SSID     "Rumah.5G"
#define WIFI_PASS     "Broklyn1985"
#define WIFI_TIMEOUT  20000  // ms to wait for WiFi connection

// === MQTT (HiveMQ Cloud) ===
#define MQTT_HOST     "d34d0f0f7c86418d8c0f451a884ff4e7.s1.eu.hivemq.cloud"
#define MQTT_PORT     8883
#define MQTT_USER     "JARVIS"
#define MQTT_PASS     "iloveyou3000"

// === Device ID (auto-generated from ESP chip ID) ===
// This is used as the unique MQTT topic identifier
// Format: p10_<hex_chip_id>/commands and p10_<hex_chip_id>/status

// === Timing ===
#define STATUS_INTERVAL    30000  // heartbeat every 30s
#define RECONNECT_INTERVAL 5000   // MQTT reconnect every 5s
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     7      // GMT+7 (WIB) - adjust for your timezone
#define NTP_DAYLIGHT       0

// === Let's Encrypt Root CA (ISRG Root X1) for TLS ===
static const char CA_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6
UA5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+s
WT8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3q
yHB5T0Y3HsLuJvW5iB4YlcNHlsdu87kGEB581Tj2djZYWMj2zqqSBhdO2dpJdXk
lKfJsEEWinogViHfHRwGBMl5iiNoKU/EMI99Vf0wAh3/CzhWfMaOVGfnECEaWw5l
fBIw7VdP0BTiGkRsVxw3sINLjPvL6IUfpiFQoDqJnGkmt4dIb8IGJbx8V8PZ4nZ
Fhp0JJ1PCIIUdWcA7D58LMvLD7Lm8Ubb6Z6SUcOsFY1PqBLfNQ6NJnFgRPgaQz8
fD1KV7PmEJDYFT9HmgSNA6rU0Mb2mM9FIydBnEM3T/Rq3xK78P1g+OB9v69Ye8W
dCbfx3B5qdXiFd7gMn1FNJqjWRN2EBYaJq8vNRkmK41q4bBb+bEibyBJZYfk2V+
CB4nB7NW+J1S4KXiU4Qb797DH2sY9HfCf1h7QaT3pCKQ2Mj35pnNTd3jnwiSxI7
jGFMjib2wy8X0d8fQ1p4JvxDfK6MXfN0AFNbdN1U7L2dkS4Qad4YhlIDNWKC0O
LBo5vQ49OG5l3YJ13YibMO+kV8YGhANyBy6vwSF6ZF3GB0QB0djQoXrS3IHQwDQ
YJKoZIhvcNAQELBQADggIBAC5Bsh+Y3bYtOKsV2JMOaYkXnGHFhG9JOPKq1KvV
bMHMB9AD/TatP1bHO5U3bP+hP9Xz6fH2G0dVFiG0lPwUF1M1O+BdG2M0b58YWcI
S9YfZbBvj5eqcXCnMF4V8J9r0vFhKsJijvYkXRi1f3S7XH7KC7O0jNH69JU2FI
3k6FNGYyK3bL4HNQBMLbG7m7F7pK18TmEcHNIckshvOa04GQ9dWVwhsJ3W4K8
-----END CERTIFICATE-----
)EOF";

#endif
