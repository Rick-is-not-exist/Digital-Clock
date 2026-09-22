/*
  =============================================================================
  PROYEK: P10 IoT Neon Clock & Running Text Controller (ESP8266 + DMDESP)
  MODE: WiFi AP Mode (Captive Portal) + STA Mode + MQTT via HiveMQ Cloud
  FUNGSI:
   - AP Mode: captive portal untuk setup WiFi pertama kali
   - STA Mode: koneksi ke WiFi client + MQTT cloud
   - NTP time sync otomatis
   - Panel P10 display: jam, running text, animasi
  =============================================================================
  
  Koneksi Pin ESP8266 (NodeMCU) ke Panel P10 (DMD):
  - D0 (GPIO16) -> Pin A
  - D5 (GPIO14) -> Pin CLK
  - D6 (GPIO12) -> Pin B
  - D7 (GPIO13) -> Pin R (Data / R_DATA)
  - D8 (GPIO15) -> Pin NOE / OE (Brightness PWM)
  - D4 (GPIO2)  -> Pin SCLK / LATCH
  - GND         -> GND Panel P10 & Power Supply 5V eksternal
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Ticker.h>
#include <time.h>
#include <ArduinoJson.h>
#include <DMDESP.h>
#include <fonts/Mono5x7.h>
#include <fonts/ElektronMart5x6.h>
#include <fonts/EMSans8x16.h>
#include "config.h"

extern "C" {
  #include "user_interface.h"
}

// WiFi & MQTT
WiFiClientSecure wifiSecure;
PubSubClient mqtt(wifiSecure);
ESP8266WebServer server(80);
DNSServer dnsServer;

// DMD (1 Panel P10 horizontal x 1 vertical)
#define DISPLAYS_WIDE 1
#define DISPLAYS_HIGH 1
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

// Timer for automatic display refresh (runs during yield() inside BearSSL)
Ticker dmdRefreshTimer;

// Device UID (unique identifier from ESP chip ID)
String deviceUID;

// Saved WiFi credentials
String savedSSID = "";
String savedPass = "";
bool hasSavedWiFi = false;
bool apMode = false;

// Variables Pengaturan
String text1 = "HALLO";
String anim = "scroll_left";
int speed_ms = 40;
int clock_duration = 10;
int text_duration = 15;
String display_mode = "cycle";
int brightness_pwm = 51;
bool format_24h = true;
bool show_seconds = true;
bool panel_power = true;
bool displayDirty = true;

// Variabel Waktu Internal
int current_hour = 12;
int current_min = 0;
int current_sec = 0;
int last_display_sec = -1;
int current_day = 1;
int current_month = 1;
int current_year = 2026;
unsigned long last_sec_tick = 0;
unsigned long last_mode_switch = 0;
bool is_showing_clock = true;
bool colon_visible = true;

// BARIS 2: Day scroll + Date static state machine
enum DateDisplayState { SCROLL_DAY, DATE_STATIC };
DateDisplayState dateState = SCROLL_DAY;
int dateScrollX = 32;
unsigned long lastDateScrollTick = 0;
unsigned long lastDateStaticTick = 0;
int dayScrollCycles = 0;
#define DAY_SCROLL_SPEED_MS 50
#define DAY_SCROLL_MAX_CYCLES 2
#define DATE_STATIC_DURATION 5000

// Running Text
int scroll_x = 32 * DISPLAYS_WIDE;
unsigned long last_scroll_tick = 0;
String last_anim = "";
String last_text1 = "";
bool fitsPanelCache = false;
bool cacheValid = false;

// Timing
unsigned long lastStatusSent = 0;
unsigned long lastNtpSync = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastMqttLoop = 0;
unsigned long lastWifiReconnect = 0;
#define NTP_RESYNC_INTERVAL 3600000
#define WIFI_RECONNECT_INTERVAL 30000
bool mqttConnected = false;

// Deferred display operations (set in MQTT callback, applied in loop)
bool pendingBrightness = false;
int pendingBrightnessVal = 0;
bool pendingPowerOn = false;
bool pendingPowerOff = false;

// Forward declarations for MQTT callbacks
void connectMQTT();
void onMqttMessage(char* topic, byte* payload, unsigned int length);

// ==================== EEPROM ====================
void saveWiFiCredentials(String ssid, String pass) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_ADDR, EEPROM_MAGIC);
  for (int i = 0; i < EEPROM_SSID_LEN; i++) {
    EEPROM.write(EEPROM_SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
  }
  for (int i = 0; i < EEPROM_PASS_LEN; i++) {
    EEPROM.write(EEPROM_PASS_ADDR + i, i < pass.length() ? pass[i] : 0);
  }
  EEPROM.commit();
  EEPROM.end();
  Serial.println("[EEPROM] WiFi credentials saved!");
}

bool loadWiFiCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(EEPROM_ADDR) != EEPROM_MAGIC) {
    EEPROM.end();
    Serial.println("[EEPROM] No saved credentials found.");
    return false;
  }
  char ssid[EEPROM_SSID_LEN + 1] = {0};
  char pass[EEPROM_PASS_LEN + 1] = {0};
  for (int i = 0; i < EEPROM_SSID_LEN; i++) ssid[i] = EEPROM.read(EEPROM_SSID_ADDR + i);
  for (int i = 0; i < EEPROM_PASS_LEN; i++) pass[i] = EEPROM.read(EEPROM_PASS_ADDR + i);
  EEPROM.end();
  savedSSID = String(ssid);
  savedPass = String(pass);
  if (savedSSID.length() > 0) {
    Serial.printf("[EEPROM] Loaded: SSID=%s\n", savedSSID.c_str());
    return true;
  }
  return false;
}

void clearWiFiCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_ADDR, 0x00);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("[EEPROM] WiFi credentials cleared.");
}

// ==================== CAPTIVE PORTAL ====================
const char* portalPage = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>P10 Clock - Setup WiFi</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: #0f0f23;
      color: #e0e0e0;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .card {
      background: #1a1a2e;
      border-radius: 16px;
      padding: 40px 32px;
      max-width: 400px;
      width: 100%;
      box-shadow: 0 8px 32px rgba(0,0,0,0.4);
      border: 1px solid #2a2a4a;
    }
    .icon { text-align: center; font-size: 48px; margin-bottom: 16px; }
    h1 { text-align: center; font-size: 20px; margin-bottom: 8px; color: #fff; }
    p { text-align: center; font-size: 14px; color: #888; margin-bottom: 24px; }
    label { display: block; font-size: 13px; color: #aaa; margin-bottom: 6px; font-weight: 500; }
    input[type="text"], input[type="password"] {
      width: 100%;
      padding: 12px 16px;
      border: 1px solid #2a2a4a;
      border-radius: 10px;
      background: #0f0f23;
      color: #fff;
      font-size: 15px;
      margin-bottom: 16px;
      outline: none;
      transition: border-color 0.2s;
    }
    input:focus { border-color: #e74c3c; }
    .toggle-pass {
      position: relative;
    }
    .toggle-pass input { padding-right: 44px; }
    .toggle-btn {
      position: absolute;
      right: 8px;
      top: 50%;
      transform: translateY(-50%);
      background: none;
      border: none;
      color: #888;
      cursor: pointer;
      font-size: 18px;
      padding: 4px 8px;
    }
    button[type="submit"] {
      width: 100%;
      padding: 14px;
      border: none;
      border-radius: 10px;
      background: #e74c3c;
      color: #fff;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: background 0.2s;
    }
    button[type="submit"]:hover { background: #c0392b; }
    .note {
      text-align: center;
      font-size: 12px;
      color: #555;
      margin-top: 16px;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="icon">&#128337;</div>
    <h1>P10 IoT Clock</h1>
    <p>Hubungkan ke WiFi rumah anda</p>
    <form action="/save" method="POST">
      <label for="ssid">Nama WiFi (SSID)</label>
      <input type="text" id="ssid" name="ssid" placeholder="Masukkan nama WiFi" required>
      <label for="pass">Password WiFi</label>
      <div class="toggle-pass">
        <input type="password" id="pass" name="pass" placeholder="Masukkan password WiFi">
        <button type="button" class="toggle-btn" onclick="togglePass()">&#128065;</button>
      </div>
      <button type="submit">Simpan & Hubungkan</button>
    </form>
    <div class="note">WiFi akan otomatis tersimpan. ESP akan restart setelah terhubung.</div>
  </div>
  <script>
    function togglePass() {
      var x = document.getElementById("pass");
      x.type = x.type === "password" ? "text" : "password";
    }
  </script>
</body>
</html>
)rawliteral";

const char* successPage = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>P10 Clock - Tersimpan!</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: #0f0f23;
      color: #e0e0e0;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .card {
      background: #1a1a2e;
      border-radius: 16px;
      padding: 40px 32px;
      max-width: 400px;
      width: 100%;
      text-align: center;
      box-shadow: 0 8px 32px rgba(0,0,0,0.4);
      border: 1px solid #2a2a4a;
    }
    .icon { font-size: 64px; margin-bottom: 16px; }
    h1 { font-size: 20px; margin-bottom: 12px; color: #2ecc71; }
    p { font-size: 14px; color: #888; line-height: 1.6; }
  </style>
</head>
<body>
  <div class="card">
    <div class="icon">&#9989;</div>
    <h1>WiFi Tersimpan!</h1>
    <p>ESP akan restart dan terhubung ke WiFi anda dalam beberapa detik.<br><br>
    Jika tidak terhubung otomatis, restart ESP manual.</p>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", portalPage);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  
  if (ssid.length() == 0) {
    server.send(400, "text/html", "<h1>SSID tidak boleh kosong!</h1><a href='/'>Kembali</a>");
    return;
  }

  Serial.printf("[PORTAL] Saving SSID=%s\n", ssid.c_str());
  saveWiFiCredentials(ssid, pass);
  server.send(200, "text/html", successPage);
  
  delay(2000);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

void startCaptivePortal() {
  apMode = true;
  Serial.println("[PORTAL] Starting AP Mode...");
  
  WiFi.mode(WIFI_AP);
  String apSSID = "P10-" + deviceUID;
  WiFi.softAP(apSSID.c_str(), AP_PASS);
  
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[PORTAL] AP SSID: %s\n", apSSID.c_str());
  Serial.printf("[PORTAL] AP IP: %s\n", apIP.toString().c_str());
  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);
  
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("[PORTAL] Captive portal ready! Open browser and connect to AP.");
  
  dmd.clear();
  dmd.setFont(ElektronMart5x6);
  dmd.drawText(2, 4, "SETP", 4);
  dmd.swapBuffers();
}

void handlePortalClient() {
  dnsServer.processNextRequest();
  server.handleClient();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== P10 IoT Clock (MQTT Mode) ===");

  // Generate device UID from chip ID
  deviceUID = String(ESP.getChipId(), HEX);
  Serial.print("Device UID: ");
  Serial.println(deviceUID);

  // Initialize DMD
  dmd.setDoubleBuffer(true);
  dmd.start();
  dmd.setBrightness(brightness_pwm);
  dmd.setFont(EMSans8x16);
  dmd.clear();

  // Auto-refresh display via Ticker — fires during yield() inside BearSSL
  dmdRefreshTimer.attach_ms(2, []() { dmd.loop(); });

  // Show connecting message on panel
  dmd.setFont(ElektronMart5x6);
  dmd.drawText(2, 4, "INIT", 4);
  dmd.swapBuffers();

  // Load saved WiFi credentials from EEPROM
  hasSavedWiFi = loadWiFiCredentials();

  if (hasSavedWiFi) {
    // Try to connect with saved credentials
    Serial.println("[WIFI] Trying saved credentials...");
    connectWiFi(savedSSID, savedPass);
  }

  // If still not connected, start captive portal
  if (WiFi.status() != WL_CONNECTED) {
    if (!hasSavedWiFi) {
      Serial.println("[WIFI] No saved credentials found.");
    } else {
      Serial.println("[WIFI] Saved credentials failed.");
    }
    startCaptivePortal();
    return; // Skip MQTT, NTP, etc. — stay in AP mode
  }

  // Setup NTP and wait for sync
  Serial.println("[NTP] Configuring time sync...");
  configTime(NTP_GMT_OFFSET * 3600, NTP_DAYLIGHT, NTP_SERVER);

  Serial.println("[NTP] Waiting for time sync...");
  time_t now = time(nullptr);
  unsigned long ntpStart = millis();
  while (now < 8 * 3600 && millis() - ntpStart < NTP_WAIT_TIME) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  if (now >= 8 * 3600) {
    Serial.println("\n[NTP] Time synced!");
    struct tm* t = localtime(&now);
    current_hour = t->tm_hour;
    current_min = t->tm_min;
    current_sec = t->tm_sec;
    current_day = t->tm_mday;
    current_month = t->tm_mon + 1;
    current_year = t->tm_year + 1900;
    Serial.printf("[NTP] Time set to: %02d:%02d:%02d %02d/%02d/%04d\n",
                  current_hour, current_min, current_sec,
                  current_day, current_month, current_year);
  } else {
    Serial.println("\n[NTP] Sync timeout, continuing...");
  }

  // Setup MQTT
  String clientId = "p10_" + deviceUID;
  String statusTopic = "p10/" + deviceUID + "/status";
  String cmdTopic = "p10/" + deviceUID + "/commands";

  wifiSecure.setInsecure();
  wifiSecure.setBufferSizes(512, 512);  // Reduce TLS record size to minimize blocking time
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(60);  // Reduce keepalive traffic frequency

  // Connect to MQTT
  connectMQTT();

  lastNtpSync = millis();
  last_mode_switch = millis();
  lastStatusSent = millis();
  Serial.println("[SETUP] Ready!");
}

// ==================== WIFI ====================
void connectWiFi(String ssid, String pass) {
  Serial.printf("[WIFI] Connecting to %s", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.setOutputPower(20);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" FAILED! Will retry...");
  }
}

// ==================== MQTT ====================
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;

  String clientId = "p10_" + deviceUID;
  String statusTopic = "p10/" + deviceUID + "/status";
  String cmdTopic = "p10/" + deviceUID + "/commands";

  Serial.println("[MQTT] Connecting...");
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                   statusTopic.c_str(), 1, true, "{\"online\":false}")) {
    mqttConnected = true;
    Serial.println("[MQTT] Connected!");

    mqtt.subscribe(cmdTopic.c_str(), 1);
    Serial.printf("[MQTT] Subscribed to %s\n", cmdTopic.c_str());

    mqtt.publish(statusTopic.c_str(), "{\"online\":true}", true);
    Serial.println("[MQTT] Ready!");
  } else {
    Serial.printf("[MQTT] Connect failed, rc=%d\n", mqtt.state());
    mqttConnected = false;
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.printf("[MQTT] Received on %s: %s\n", topic, msg);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    Serial.printf("[MQTT] JSON parse error: %s\n", error.c_str());
    return;
  }

  // Apply settings
  if (doc.containsKey("text1")) text1 = doc["text1"].as<String>();
  if (doc.containsKey("anim")) anim = doc["anim"].as<String>();
  if (doc.containsKey("speed_ms")) speed_ms = constrain(doc["speed_ms"].as<int>(), 10, 200);
  if (doc.containsKey("clock_duration")) clock_duration = constrain(doc["clock_duration"].as<int>(), 3, 300);
  if (doc.containsKey("text_duration")) text_duration = constrain(doc["text_duration"].as<int>(), 3, 300);
  if (doc.containsKey("mode")) display_mode = doc["mode"].as<String>();
  if (doc.containsKey("brightness_pwm")) {
    pendingBrightnessVal = constrain(doc["brightness_pwm"].as<int>(), 0, 100);
    pendingBrightness = true;
  }
  if (doc.containsKey("format_24h")) format_24h = doc["format_24h"].as<bool>();
  if (doc.containsKey("show_seconds")) show_seconds = doc["show_seconds"].as<bool>();

  if (doc.containsKey("power")) {
    bool newPower = doc["power"].as<bool>();
    if (newPower && !panel_power) {
      pendingPowerOn = true;
      Serial.println("[MQTT] Panel ON pending");
    } else if (!newPower && panel_power) {
      pendingPowerOff = true;
      Serial.println("[MQTT] Panel OFF pending");
    }
  }

  cacheValid = false;
  displayDirty = true;
  Serial.println("[MQTT] Settings applied!");
}

// ==================== STATUS ====================
void sendStatus() {
  if (!mqttConnected) return;
  String statusTopic = "p10/" + deviceUID + "/status";
  mqtt.publish(statusTopic.c_str(), "{\"online\":true}", false);
}

// ==================== DISPLAY LOGIC ====================
bool checkTextFitsPanel(const char* str, int len) {
  dmd.setFont(Mono5x7);
  int w = dmd.textWidth(str, len);
  return (w <= 32 * DISPLAYS_WIDE);
}

void updateClockFromNTP() {
  if (millis() - last_sec_tick >= 1000) {
    last_sec_tick = millis();
    colon_visible = !colon_visible;

    time_t now = time(nullptr);
    if (now >= 8 * 3600) {
      // NTP synced — read directly from SNTP
      struct tm* t = localtime(&now);
      current_hour = t->tm_hour;
      current_min = t->tm_min;
      current_sec = t->tm_sec;
      current_day = t->tm_mday;
      current_month = t->tm_mon + 1;
      current_year = t->tm_year + 1900;
    } else {
      // NTP not yet synced — fallback manual increment
      current_sec++;
      if (current_sec >= 60) {
        current_sec = 0;
        current_min++;
        if (current_min >= 60) {
          current_min = 0;
          current_hour++;
          if (current_hour >= 24) current_hour = 0;
        }
        }
      }
      } // end text1.length() > 0
    }
  }

const char* getDayName(int day) {
  const char* days[] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
  if (day >= 0 && day <= 6) return days[day];
  return "???";
}

const char* getMonthName(int month) {
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "Mei", "Jun", "Jul", "Ags", "Sep", "Okt", "Nov", "Des"};
  if (month >= 1 && month <= 12) return months[month - 1];
  return "???";
}

int getDayOfWeek(int y, int m, int d) {
  if (m < 3) { m += 12; y--; }
  int k = y % 100;
  int j = y / 100;
  int dow = (d + (13*(m+1))/5 + k + k/4 + j/4 + 5*j) % 7;
  return (dow + 6) % 7;
}

void renderClockOnP10() {
  char hourBuff[4];
  char minBuff[4];
  char colonChar[2] = ":";
  
  int h = current_hour;
  if (!format_24h) {
    h = current_hour % 12;
    if (h == 0) h = 12;
  }
  
  sprintf(hourBuff, "%02d", h);
  sprintf(minBuff, "%02d", current_min);
  
  // BARIS 1: HH:MM (Mono5x7, centered)
  dmd.setFont(Mono5x7);
  int hourW = dmd.textWidth(hourBuff, strlen(hourBuff));
  int colonW = dmd.textWidth(colonChar, 1);
  int minW = dmd.textWidth(minBuff, strlen(minBuff));
  int totalW = hourW + 1 + colonW + 1 + minW;
  int startX = (32 - totalW) / 2;
  if (startX < 0) startX = 0;
  
  dmd.drawText(startX, 0, hourBuff, strlen(hourBuff));
  
  if (colon_visible) {
    dmd.drawText(startX + hourW + 1, 0, colonChar, 1);
  }
  
  dmd.drawText(startX + hourW + 1 + colonW + 1, 0, minBuff, strlen(minBuff));
  
  // BARIS 2: Day scroll + Date static state machine
  dmd.setFont(Mono5x7);
  
  if (dateState == SCROLL_DAY) {
    int dow = getDayOfWeek(current_year, current_month, current_day);
    char dayBuff[12];
    sprintf(dayBuff, "%s", getDayName(dow));
    int dayW = dmd.textWidth(dayBuff, strlen(dayBuff));
    
    if (millis() - lastDateScrollTick >= DAY_SCROLL_SPEED_MS) {
      lastDateScrollTick = millis();
      dateScrollX--;
    }
    
    dmd.drawText(dateScrollX, 9, dayBuff, strlen(dayBuff));
    
    if (dateScrollX < -dayW) {
      dateScrollX = 32;
      dayScrollCycles++;
      if (dayScrollCycles >= DAY_SCROLL_MAX_CYCLES) {
        dateState = DATE_STATIC;
        lastDateStaticTick = millis();
        dayScrollCycles = 0;
      }
    }
  } else {
    char dateBuff[12];
    sprintf(dateBuff, "%02d/%02d", current_day, current_month);
    int dateW = dmd.textWidth(dateBuff, strlen(dateBuff));
    int dateX = (32 - dateW) / 2;
    dmd.drawText(dateX, 9, dateBuff, strlen(dateBuff));
    
    if (millis() - lastDateStaticTick >= DATE_STATIC_DURATION) {
      dateState = SCROLL_DAY;
      dateScrollX = 32;
      lastDateScrollTick = millis();
    }
  }
}

// ==================== LOOP ====================
void loop() {

  // AP Mode — handle captive portal
  if (apMode) {
    handlePortalClient();
    return;
  }

  // WiFi reconnect (throttled)
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiReconnect > WIFI_RECONNECT_INTERVAL) {
      lastWifiReconnect = millis();
      connectWiFi(savedSSID, savedPass);
    }
  }

  // MQTT reconnect (manual, throttled)
  if (!mqtt.connected()) {
    mqttConnected = false;
    if (millis() - lastMqttReconnect > RECONNECT_INTERVAL) {
      lastMqttReconnect = millis();
      connectMQTT();
    }
  } else {
    // Throttle mqtt.loop() — TLS processing can block 50-200ms, avoid freezing display
    if (millis() - lastMqttLoop >= 200) {
      lastMqttLoop = millis();
      mqtt.loop();
    }
  }

  // Send heartbeat
  if (millis() - lastStatusSent > STATUS_INTERVAL) {
    lastStatusSent = millis();
    sendStatus();
  }

  // Periodic NTP re-sync (every hour)
  if (millis() - lastNtpSync > NTP_RESYNC_INTERVAL) {
    lastNtpSync = millis();
    time_t now = time(nullptr);
    if (now > 8 * 3600) {
      struct tm* t = localtime(&now);
      current_hour = t->tm_hour;
      current_min = t->tm_min;
      current_sec = t->tm_sec;
      current_day = t->tm_mday;
      current_month = t->tm_mon + 1;
      current_year = t->tm_year + 1900;
      Serial.printf("[NTP] Re-synced: %02d:%02d:%02d\n", current_hour, current_min, current_sec);
    }
  }

  // Display logic (only when panel is on)
  if (panel_power) {
    updateClockFromNTP();

    unsigned long currentMillis = millis();
    unsigned long activeDuration = (is_showing_clock ? clock_duration : text_duration) * 1000;

    if (display_mode == "cycle") {
      if (currentMillis - last_mode_switch >= activeDuration) {
        last_mode_switch = currentMillis;
        is_showing_clock = !is_showing_clock;
        scroll_x = 32 * DISPLAYS_WIDE;
        last_scroll_tick = millis();
        displayDirty = true;
        if (is_showing_clock) {
          dateState = SCROLL_DAY;
          dateScrollX = 32;
          dayScrollCycles = 0;
          lastDateScrollTick = millis();
        }
      }
    } else if (display_mode == "clock_only") {
      is_showing_clock = true;
    } else if (display_mode == "text_only") {
      is_showing_clock = false;
    }

    if (is_showing_clock) {
      bool secChanged = (current_sec != last_display_sec);
      bool scrollTick = (dateState == SCROLL_DAY && currentMillis - lastDateScrollTick >= DAY_SCROLL_SPEED_MS);
      
      if (displayDirty || secChanged || scrollTick) {
        if (secChanged) last_display_sec = current_sec;
        dmd.clear();
        displayDirty = false;
        renderClockOnP10();
        dmd.swapBuffers();
      }
    } else {
      // Skip rendering if text is empty
      if (text1.length() == 0) {
        displayDirty = false;
      } else {
      // Recalculate cache when text/anim changes
      if (text1 != last_text1 || anim != last_anim || !cacheValid) {
        fitsPanelCache = checkTextFitsPanel(text1.c_str(), text1.length());
        last_text1 = text1;
        last_anim = anim;
        cacheValid = true;
        scroll_x = 32 * DISPLAYS_WIDE;
        last_scroll_tick = millis();
        displayDirty = true;
      }

      bool useStatic = (fitsPanelCache && anim == "static");

      if (useStatic) {
        if (displayDirty) {
          dmd.clear();
          displayDirty = false;
          dmd.setFont(Mono5x7);
          int text_width = dmd.textWidth(text1.c_str(), text1.length());
          int center_x = (32 * DISPLAYS_WIDE - text_width) / 2;
          if (center_x < 0) center_x = 0;
          int center_y = (16 - 7) / 2;
          dmd.drawText(center_x, center_y, text1.c_str(), text1.length());
          dmd.swapBuffers();
        }
      } else {
        if (millis() - last_scroll_tick >= speed_ms) {
          last_scroll_tick = millis();
          int text_width = dmd.textWidth(text1.c_str(), text1.length());
          if (anim == "scroll_right") {
            scroll_x++;
            if (scroll_x > 32 * DISPLAYS_WIDE) scroll_x = -text_width;
          } else {
            scroll_x--;
            if (scroll_x < -text_width) scroll_x = 32 * DISPLAYS_WIDE;
          }
          displayDirty = true;
        }

        if (displayDirty) {
          dmd.clear();
          dmd.setFont(EMSans8x16);
          dmd.drawText(scroll_x, 0, text1.c_str(), text1.length());
          dmd.swapBuffers();
          displayDirty = false;
        }
      }
    }
  }

  // Apply deferred display operations from MQTT callback (safe from ISR)
  if (pendingBrightness) {
    brightness_pwm = pendingBrightnessVal;
    dmd.setBrightness(brightness_pwm);
    pendingBrightness = false;
  }
  if (pendingPowerOn) {
    panel_power = true;
    dmd.setBrightness(brightness_pwm);
    displayDirty = true;
    pendingPowerOn = false;
    Serial.println("[MQTT] Panel ON");
  }
  if (pendingPowerOff) {
    panel_power = false;
    dmd.setBrightness(0);
    dmd.clear();
    dmd.swapBuffers();
    pendingPowerOff = false;
    Serial.println("[MQTT] Panel OFF");
  }
}
