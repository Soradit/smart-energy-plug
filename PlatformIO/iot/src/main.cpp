/*
 * Smart Energy Plug — ESP32 + PZEM-004T (Simulated ×3)
 * จำลองอุปกรณ์ 3 ชิ้นแยกกัน → ส่ง MQTT topic แยก
 *   pzem/esp32/fridge   — ตู้เย็น
 *   pzem/esp32/ac       — แอร์
 *   pzem/esp32/computer — คอมพิวเตอร์
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "config.h"
#include "energy.h"

// ══════════════════════════════════════════════════════════
//  อุปกรณ์จำลอง 3 ชิ้น — ค่าไฟต่างกัน
// ══════════════════════════════════════════════════════════
struct Device {
  const char* name;
  const char* topic;
  float       baseWatt;   // W ปกติของอุปกรณ์นี้
  float       energyKwh;  // สะสม kWh
  unsigned long lastMs;
  EnergyTracker* tracker;
};

EnergyTracker trFridge(RATE_THB);
EnergyTracker trAC(RATE_THB);
EnergyTracker trPC(RATE_THB);

Device devices[3] = {
  { "fridge",   "pzem/esp32/fridge",   150,  0, 0, &trFridge },
  { "ac",       "pzem/esp32/ac",       1200, 0, 0, &trAC     },
  { "computer", "pzem/esp32/computer", 200,  0, 0, &trPC     },
};

// ══════════════════════════════════════════════════════════
//  SPIFFS — เก็บบันทึกรายวัน (คล้าย SQLite แต่ใช้ JSON)
//  ไฟล์แยกต่ออุปกรณ์: /log_fridge.json, /log_ac.json, ...
//  Schema: [ {date, kwh, cost, peak_w, samples}, ... ]
// ══════════════════════════════════════════════════════════
#define MAX_DAYS 30

struct DayRow {
  char  date[12];
  float kwh;
  float cost;
  float peakW;
  int   n;
};

String dbPath(const char* devName) {
  return String("/log_") + devName + ".json";
}

int dbLoad(const char* dev, DayRow* rows) {
  String path = dbPath(dev);
  if (!SPIFFS.exists(path)) return 0;
  File f = SPIFFS.open(path, "r");
  if (!f) return 0;
  JsonDocument doc;
  if (deserializeJson(doc, f)) { f.close(); return 0; }
  f.close();
  int n = 0;
  for (JsonObject o : doc.as<JsonArray>()) {
    if (n >= MAX_DAYS) break;
    strlcpy(rows[n].date, o["d"] | "", 12);
    rows[n].kwh  = o["k"] | 0.0f;
    rows[n].cost = o["c"] | 0.0f;
    rows[n].peakW= o["p"] | 0.0f;
    rows[n].n    = o["n"] | 0;
    n++;
  }
  return n;
}

void dbSave(const char* dev, DayRow* rows, int count) {
  File f = SPIFFS.open(dbPath(dev), "w");
  if (!f) return;
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < count; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["d"] = rows[i].date;
    o["k"] = rows[i].kwh;
    o["c"] = rows[i].cost;
    o["p"] = rows[i].peakW;
    o["n"] = rows[i].n;
  }
  serializeJson(doc, f);
  f.close();
}

// INSERT OR UPDATE row ของวันนี้
void dbUpsert(const char* dev, const char* today, float kwh, float cost, float watt) {
  DayRow rows[MAX_DAYS];
  int count = dbLoad(dev, rows);
  int idx = -1;
  for (int i = 0; i < count; i++) {
    if (strcmp(rows[i].date, today) == 0) { idx = i; break; }
  }
  if (idx < 0) {
    if (count >= MAX_DAYS) {
      memmove(rows, rows+1, (MAX_DAYS-1)*sizeof(DayRow));
      count = MAX_DAYS-1;
    }
    idx = count++;
    strlcpy(rows[idx].date, today, 12);
    rows[idx] = {0};
    strlcpy(rows[idx].date, today, 12);
  }
  rows[idx].kwh  = kwh;
  rows[idx].cost = cost;
  rows[idx].n++;
  if (watt > rows[idx].peakW) rows[idx].peakW = watt;
  dbSave(dev, rows, count);
}

// ══════════════════════════════════════════════════════════
//  วันที่จำลอง (Wokwi: 5 นาที = 1 วัน)
// ══════════════════════════════════════════════════════════
char simDate[12] = "2024-01-01";
int  simDayNum = 1;
unsigned long dayTick = 0;

void tickDate() {
  if (millis() - dayTick >= 300000UL) {
    dayTick = millis();
    if (++simDayNum > 31) simDayNum = 1;
    snprintf(simDate, 12, "2024-01-%02d", simDayNum);
    Serial.printf("[DATE] new day: %s\n", simDate);
    // reset tracker
    for (auto& d : devices) d.tracker->begin(d.energyKwh);
  }
}

// ══════════════════════════════════════════════════════════
//  Simulate + Publish อุปกรณ์หนึ่งตัว
// ══════════════════════════════════════════════════════════
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

void publishDevice(Device& dev) {
  // จำลองค่า sensor
  float voltage = 218.0f + random(0, 50) * 0.1f;
  float watt    = dev.baseWatt * (0.85f + random(0,30)*0.01f);
  float current = watt / voltage;
  float pf      = 0.85f + random(0,15)*0.01f;
  float freq    = 50.0f + random(-2,2)*0.1f;

  // สะสม kWh
  unsigned long now = millis();
  if (dev.lastMs > 0)
    dev.energyKwh += watt * ((now - dev.lastMs) / 3600000.0f) / 1000.0f;
  dev.lastMs = now;

  dev.tracker->update(dev.energyKwh);
  float costDay   = dev.tracker->dailyCost(dev.energyKwh);
  float costMonth = dev.tracker->monthlyCost(dev.energyKwh);

  // บันทึก DB ทุก message
  dbUpsert(dev.name, simDate, dev.tracker->dailyKwh(dev.energyKwh), costDay, watt);

  // JSON
  JsonDocument doc;
  doc["device"]     = dev.name;
  doc["voltage"]    = String(voltage, 1);
  doc["current"]    = String(current, 3);
  doc["power"]      = String(watt,    1);
  doc["energy_kwh"] = String(dev.energyKwh, 5);
  doc["frequency"]  = String(freq,    1);
  doc["pf"]         = String(pf,      2);
  doc["cost_today"] = String(costDay, 2);
  doc["cost_month"] = String(costMonth, 0);
  doc["level"]      = EnergyTracker::classify(watt);
  doc["date"]       = simDate;
  doc["ts"]         = now;

  char buf[400];
  serializeJson(doc, buf);
  bool ok = mqtt.publish(dev.topic, buf);
  Serial.printf("[%s][%s] %.1fV %.3fA %.1fW %.2f฿/day\n",
    ok?"PUB":"ERR", dev.name, voltage, current, watt, costDay);
}

// ══════════════════════════════════════════════════════════
//  WiFi / MQTT
// ══════════════════════════════════════════════════════════
void connectWiFi() {
  Serial.printf("WiFi %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 20; i++) {
    delay(500); Serial.print(".");
  }
  Serial.println(WiFi.status()==WL_CONNECTED ? " OK" : " FAIL");
}

void mqttConnect() {
  for (int i = 0; !mqtt.connected() && i < 5; i++) {
    if (mqtt.connect(MQTT_CLIENT)) {
      mqtt.publish(TOPIC_STATUS, "{\"state\":\"online\"}", true);
      Serial.println("[MQTT] connected");
    } else { delay(3000); }
  }
}

// ══════════════════════════════════════════════════════════
unsigned long lastPub = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Smart Energy Plug (×3 sim) ===");

  if (!SPIFFS.begin(true)) { SPIFFS.format(); SPIFFS.begin(true); }
  Serial.printf("[FS] free %d B\n", SPIFFS.totalBytes()-SPIFFS.usedBytes());

  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(512);
  mqttConnect();

  for (auto& d : devices) d.tracker->begin(0.0f);
  dayTick = millis();
  Serial.println("Ready!\n");
}

void loop() {
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();
  tickDate();

  if (millis() - lastPub >= 2000) {
    lastPub = millis();
    for (auto& d : devices) publishDevice(d);
  }
}
