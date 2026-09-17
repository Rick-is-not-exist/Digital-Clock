/*
  =============================================================================
  PROYEK: P10 IoT Neon Clock & Running Text Controller (ESP8266 + DMDESP)
  MODE: WiFi Station (STA) + MQTT via HiveMQ Cloud
  FUNGSI:
   - Koneksi ke WiFi rumah (Station Mode)
   - MQTT subscriber untuk menerima perintah dari cloud
   - MQTT publisher untuk mengirim status heartbeat
   - NTP time sync otomatis dari internet
   - Tidak ada web server lokal (frontend di-hosting di Netlify)
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
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <DMDESP.h>
#include <fonts/Mono5x7.h>
#include <fonts/ElektronMart5x6.h>
#include <fonts/EMSans8x16.h>
#include "config.h"

// WiFi & MQTT
WiFiClientSecure wifiSecure;
PubSubClient mqtt(wifiSecure);

// DMD (1 Panel P10 horizontal x 1 vertical)
#define DISPLAYS_WIDE 1
#define DISPLAYS_HIGH 1
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

// Device UID (unique identifier from ESP chip ID)
String deviceUID;

// Variables Pengaturan
String text1 = "SELAMAT DATANG DI SISTEM IoT P10 ESP8266";
String anim = "scroll_left";
int speed_ms = 40;
int clock_duration = 10;
int text_duration = 8;
String display_mode = "cycle";
int brightness_pwm = 204;
bool format_24h = true;
bool show_seconds = true;

// Variabel Waktu Internal
int current_hour = 12;
int current_min = 0;
int current_sec = 0;
int current_day = 1;
int current_month = 1;
int current_year = 2026;
unsigned long last_sec_tick = 0;
unsigned long last_mode_switch = 0;
bool is_showing_clock = true;
bool colon_visible = true;
bool date_show_day = true;
unsigned long last_date_switch = 0;

// Running Text
int scroll_x = 32 * DISPLAYS_WIDE;
unsigned long last_scroll_tick = 0;
String last_anim = "";
String last_text1 = "";
bool fitsPanelCache = false;
bool cacheValid = false;

// Timing
unsigned long lastStatusSent = 0;
unsigned long lastMqttReconnect = 0;
bool mqttConnected = false;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== P10 IoT Clock (MQTT Mode) ===");

  // Generate device UID from chip ID
  deviceUID = String(ESP.getChipId(), HEX);
  Serial.print("Device UID: p10_");
  Serial.println(deviceUID);

  // Initialize DMD
  dmd.start();
  dmd.setBrightness(brightness_pwm);
  dmd.setFont(EMSans8x16);
  dmd.clear();

  // Show connecting message on panel
  dmd.setFont(ElektronMart5x6);
  dmd.drawText(2, 4, "INIT", 4);

  // Connect to WiFi
  connectWiFi();

  // Setup NTP
  Serial.println("[NTP] Configuring time sync...");
  configTime(NTP_GMT_OFFSET * 3600, NTP_DAYLIGHT, NTP_SERVER);

  // Setup TLS (Let's Encrypt CA)
  wifiSecure.setTrustAnchors(new BearSSL::X509List(CA_CERT));
  wifiSecure.setBufferSizes(4096, 512);

  // Setup MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);

  // Connect to MQTT
  connectMQTT();

  Serial.println("[SETUP] Ready!");
}

// ==================== WIFI ====================
void connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

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

  String clientId = "p10_" + deviceUID;
  String statusTopic = "p10/" + deviceUID + "/status";
  String cmdTopic = "p10/" + deviceUID + "/commands";

  Serial.printf("[MQTT] Connecting to %s:%d...\n", MQTT_HOST, MQTT_PORT);

  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                    statusTopic.c_str(), 1, true, "{\"online\":false}")) {
    Serial.println("[MQTT] Connected!");
    mqttConnected = true;

    // Subscribe to commands topic
    mqtt.subscribe(cmdTopic.c_str(), 1);
    Serial.printf("[MQTT] Subscribed to %s\n", cmdTopic.c_str());

    // Publish online status
    mqtt.publish(statusTopic.c_str(), "{\"online\":true}", true);

    // Show success on panel
    dmd.clear();
    dmd.setFont(ElektronMart5x6);
    dmd.drawText(2, 4, "CONN", 4);
    delay(1000);
    dmd.clear();
  } else {
    Serial.printf("[MQTT] Failed, rc=%d\n", mqtt.state());
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
  if (doc.containsKey("speed_ms")) speed_ms = doc["speed_ms"].as<int>();
  if (doc.containsKey("clock_duration")) clock_duration = doc["clock_duration"].as<int>();
  if (doc.containsKey("text_duration")) text_duration = doc["text_duration"].as<int>();
  if (doc.containsKey("mode")) display_mode = doc["mode"].as<String>();
  if (doc.containsKey("brightness_pwm")) {
    brightness_pwm = doc["brightness_pwm"].as<int>();
    dmd.setBrightness(brightness_pwm);
  }
  if (doc.containsKey("format_24h")) format_24h = doc["format_24h"].as<bool>();
  if (doc.containsKey("show_seconds")) show_seconds = doc["show_seconds"].as<bool>();

  // Time sync
  if (doc.containsKey("hour")) current_hour = doc["hour"].as<int>();
  if (doc.containsKey("min")) current_min = doc["min"].as<int>();
  if (doc.containsKey("sec")) current_sec = doc["sec"].as<int>();
  if (doc.containsKey("day")) current_day = doc["day"].as<int>();
  if (doc.containsKey("month")) current_month = doc["month"].as<int>();
  if (doc.containsKey("year")) current_year = doc["year"].as<int>();

  // Also check time_sync nested object
  if (doc.containsKey("time_sync")) {
    JsonObject t = doc["time_sync"];
    current_hour = t["hour"] | current_hour;
    current_min = t["min"] | current_min;
    current_sec = t["sec"] | current_sec;
    current_day = t["day"] | current_day;
    current_month = t["month"] | current_month;
    current_year = t["year"] | current_year;
  }

  Serial.println("[MQTT] Settings applied!");
}

void sendStatus() {
  if (!mqttConnected) return;
  String statusTopic = "p10/" + deviceUID + "/status";
  mqtt.publish(statusTopic.c_str(), "{\"online\":true}");
}

// ==================== DISPLAY LOGIC ====================
bool checkTextFitsPanel(const char* str, int len) {
  dmd.setFont(ElektronMart5x6);
  int w = dmd.textWidth(str, len);
  return (w <= 32 * DISPLAYS_WIDE);
}

void updateClockTicks() {
  if (millis() - last_sec_tick >= 1000) {
    last_sec_tick = millis();
    current_sec++;
    colon_visible = !colon_visible;
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
  dmd.clear();
  
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
  
  // BARIS 2: Tanggal
  if (millis() - last_date_switch >= 5000) {
    last_date_switch = millis();
    date_show_day = !date_show_day;
  }
  
  char dateBuff[12];
  if (date_show_day) {
    int dow = getDayOfWeek(current_year, current_month, current_day);
    sprintf(dateBuff, "%s", getDayName(dow));
  } else {
    sprintf(dateBuff, "%02d/%02d", current_day, current_month);
  }
  int dateW = dmd.textWidth(dateBuff, strlen(dateBuff));
  int dateX = (32 - dateW) / 2;
  dmd.drawText(dateX, 9, dateBuff, strlen(dateBuff));
}

// ==================== LOOP ====================
void loop() {
  // WiFi reconnect
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }

  // MQTT reconnect
  if (!mqtt.connected()) {
    mqttConnected = false;
    if (millis() - lastMqttReconnect > RECONNECT_INTERVAL) {
      lastMqttReconnect = millis();
      connectMQTT();
    }
  } else {
    mqtt.loop();
  }

  // Send heartbeat
  if (millis() - lastStatusSent > STATUS_INTERVAL) {
    lastStatusSent = millis();
    sendStatus();
  }

  // Display logic
  dmd.loop();
  updateClockTicks();

  unsigned long currentMillis = millis();
  unsigned long activeDuration = (is_showing_clock ? clock_duration : text_duration) * 1000;

  if (display_mode == "cycle") {
    if (currentMillis - last_mode_switch >= activeDuration) {
      last_mode_switch = currentMillis;
      is_showing_clock = !is_showing_clock;
      dmd.clear();
    }
  } else if (display_mode == "clock_only") {
    is_showing_clock = true;
  } else if (display_mode == "text_only") {
    is_showing_clock = false;
  }

  if (is_showing_clock) {
    renderClockOnP10();
  } else {
    // Recalculate cache when text/anim changes
    if (text1 != last_text1 || anim != last_anim || !cacheValid) {
      fitsPanelCache = checkTextFitsPanel(text1.c_str(), text1.length());
      last_text1 = text1;
      last_anim = anim;
      cacheValid = true;
      scroll_x = 32 * DISPLAYS_WIDE;
      last_scroll_tick = millis();
    }

    dmd.setBrightness(brightness_pwm);
    bool useStatic = (fitsPanelCache && anim == "static");

    dmd.clear();

    if (useStatic) {
      dmd.setFont(ElektronMart5x6);
      int text_width = dmd.textWidth(text1.c_str(), text1.length());
      int center_x = (32 * DISPLAYS_WIDE - text_width) / 2;
      if (center_x < 0) center_x = 0;
      int center_y = (16 - 8) / 2;
      dmd.drawText(center_x, center_y, text1.c_str(), text1.length());
    } else {
      dmd.setFont(EMSans8x16);
      int text_width = dmd.textWidth(text1.c_str(), text1.length());
      dmd.drawText(scroll_x, 0, text1.c_str(), text1.length());

      if (millis() - last_scroll_tick >= speed_ms) {
        last_scroll_tick = millis();
        if (anim == "scroll_right") {
          scroll_x++;
          if (scroll_x > 32 * DISPLAYS_WIDE) scroll_x = -text_width;
        } else {
          scroll_x--;
          if (scroll_x < -text_width) scroll_x = 32 * DISPLAYS_WIDE;
        }
      }
    }
  }
}
