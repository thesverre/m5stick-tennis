#include <M5Unified.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include "secrets.h"
#include "web_assets.h"  // the pages in web/, gzipped at build time by tools/embed_web.py

// IMU samples are taken at a fixed rate into a ring buffer and pushed to every
// WebSocket client in small batches. New clients first get the buffered history.
constexpr uint32_t SAMPLE_INTERVAL_MS = 10;  // 100 Hz
constexpr uint32_t SEND_EVERY = 2;           // samples per message: 50 messages/s
constexpr size_t BUF_SIZE = 256;             // ~2.5 s of history

struct Sample {
  uint32_t seq;
  uint32_t t;
  float gx, gy, gz;  // deg/s
  float ax, ay, az;  // g
  uint8_t btn;       // bit 0: M5 (front) button, bit 1: side button
};

Sample buf[BUF_SIZE];
uint32_t nextSeq = 0;
uint32_t sentSeq = 0;  // samples before this have been broadcast
uint32_t lastSampleMs = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// WebSocket events arrive on the async_tcp task; newly connected client ids are
// queued here and greeted from loop() so the ring buffer is only read there.
std::vector<uint32_t> pendingClients;
portMUX_TYPE pendingMux = portMUX_INITIALIZER_UNLOCKED;

void showStatus(const String& line1, const String& line2, uint16_t color) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(color);
  M5.Display.drawString(line1, M5.Display.width() / 2, 25);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.drawString(line2, M5.Display.width() / 2, 55);
}

void connectWifi() {
  showStatus("Connecting...", WIFI_SSID, TFT_YELLOW);
  Serial.printf("Connecting to %s", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Serial.printf("Connected! IP: %s  RSSI: %d dBm\n", ip.c_str(), WiFi.RSSI());
    showStatus("m5stick.local", ip, TFT_GREEN);
  } else {
    Serial.printf("Failed, status=%d\n", WiFi.status());
    showStatus("WiFi failed", "Press A to retry", TFT_RED);
  }
}

const char* imuName() {
  switch (M5.Imu.getType()) {
    case m5::imu_mpu6886: return "MPU6886";
    case m5::imu_sh200q:  return "SH200Q";
    case m5::imu_none:    return "no IMU";
    default:              return "IMU";
  }
}

void appendSample(String& out, const Sample& s, bool first) {
  char line[128];
  snprintf(line, sizeof(line), "%s[%lu,%lu,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%u]",
           first ? "" : ",", (unsigned long)s.seq, (unsigned long)s.t,
           s.gx, s.gy, s.gz, s.ax, s.ay, s.az, s.btn);
  out += line;
}

// Message shape: {"imu":"MPU6886","s":[[seq,t,gx,gy,gz,ax,ay,az,btn],...]}
String samplesJson(uint32_t from, uint32_t to) {
  String out;
  out.reserve(64 + (to - from) * 80);
  out += "{\"imu\":\"";
  out += imuName();
  out += "\",\"s\":[";
  for (uint32_t seq = from; seq < to; seq++) {
    appendSample(out, buf[seq % BUF_SIZE], seq == from);
  }
  out += "]}";
  return out;
}

void sampleImu() {
  M5.Imu.update();
  Sample& s = buf[nextSeq % BUF_SIZE];
  s.seq = nextSeq++;
  s.t = millis();
  M5.Imu.getGyro(&s.gx, &s.gy, &s.gz);
  M5.Imu.getAccel(&s.ax, &s.ay, &s.az);
  s.btn = (M5.BtnA.isPressed() ? 1 : 0) | (M5.BtnB.isPressed() ? 2 : 0);
}

void greetPendingClients() {
  std::vector<uint32_t> ids;
  portENTER_CRITICAL(&pendingMux);
  ids.swap(pendingClients);
  portEXIT_CRITICAL(&pendingMux);

  if (ids.empty()) return;
  uint32_t oldest = nextSeq > BUF_SIZE ? nextSeq - BUF_SIZE : 0;
  String history = samplesJson(oldest, nextSeq);
  for (uint32_t id : ids) {
    ws.text(id, history);
  }
}

void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
               void*, uint8_t*, size_t) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS client #%lu connected from %s\n", (unsigned long)client->id(),
                  client->remoteIP().toString().c_str());
    portENTER_CRITICAL(&pendingMux);
    pendingClients.push_back(client->id());
    portEXIT_CRITICAL(&pendingMux);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS client #%lu disconnected\n", (unsigned long)client->id());
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  M5.begin();
  M5.Display.setRotation(1);  // landscape: 160x80
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);
  Serial.printf("IMU: %s\n", imuName());

  connectWifi();

  if (MDNS.begin("m5stick")) {
    MDNS.addService("http", "tcp", 80);
  }
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  // Every page and script is stored gzipped; the browser unpacks it
  for (const WebAsset& asset : WEB_ASSETS) {
    server.on(asset.path, HTTP_GET, [&asset](AsyncWebServerRequest* req) {
      AsyncWebServerResponse* res = req->beginResponse(200, asset.type, asset.data, asset.len);
      res->addHeader("Content-Encoding", "gzip");
      req->send(res);
    });
  }
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  M5.update();
  // Once connected, the buttons belong to the web page's air mouse
  if (WiFi.status() != WL_CONNECTED && M5.BtnA.wasPressed()) {
    connectWifi();
  }

  greetPendingClients();

  uint32_t now = millis();
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    // Advance by the interval rather than jumping to now, so the average rate stays exact
    lastSampleMs = now - lastSampleMs > 5 * SAMPLE_INTERVAL_MS ? now : lastSampleMs + SAMPLE_INTERVAL_MS;
    sampleImu();
    // While a client's send queue is full, samples wait in the ring buffer and go out in
    // the next batch, so a short WiFi stall delays them instead of dropping them
    if (ws.count() == 0) {
      sentSeq = nextSeq;
    } else if (nextSeq - sentSeq >= SEND_EVERY && ws.availableForWriteAll()) {
      uint32_t from = nextSeq - sentSeq > BUF_SIZE ? nextSeq - BUF_SIZE : sentSeq;
      ws.textAll(samplesJson(from, nextSeq));
      sentSeq = nextSeq;
    }
  }

  static uint32_t lastCleanup = 0;
  if (now - lastCleanup > 1000) {
    lastCleanup = now;
    ws.cleanupClients();
  }

  delay(1);
}
