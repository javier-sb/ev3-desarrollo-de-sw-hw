#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ==========================
// Outputs
// ==========================
const int STAT_LED_GREEN = D2;
const int STAT_LED_RED = D4;
const int STAT_LED_YELLOW = D6;

// ==========================
// Access Point + STA WiFi
// ==========================
const char* ap_ssid = "NanoESP32-DAQ";
const char* ap_password = "daq12345";

// WiFi for MQTT
const char* wifi_ssid = "tsct";
const char* wifi_password = "12345678";

// ==========================
// MQTT
// ==========================
const char* mqtt_server = "172.27.27.90";
const int mqtt_port = 1883;
const char* mqtt_topic = "lab/daq/nano1/raw";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ==========================
// Web Server
// ==========================
WebServer server(80);

// ==========================
// ADC
// ==========================
const int pinX = A0;
const int pinY = A1;
const int pinZ = A2;

// ==========================
// Sampling
// ==========================
const int sampleRate = 200;
const unsigned long sampleInterval = 1000000UL / sampleRate;

unsigned long lastSample = 0;
unsigned long sampleID = 0;

// ==========================
// Buffer (only for web UI)
// ==========================
const int BUFFER_SIZE = 200;

struct Sample {
  unsigned long id;
  int x;
  int y;
  int z;
};

Sample buffer[BUFFER_SIZE];
int writeIndex = 0;

// ==========================
// WiFi
// ==========================
void startWiFi() {
  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(wifi_ssid, wifi_password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  bool apOK = WiFi.softAP(ap_ssid, ap_password);

  if (apOK) {
    Serial.println("AP conectado.");
    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("AP fallido");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("STA conectado: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("STA fallido");
  }
}

// ==========================
// MQTT connect
// ==========================
void ensureMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;

  static unsigned long lastTry = 0;
  if (millis() - lastTry < 3000) return;
  lastTry = millis();

  String clientId = "NanoESP32-DAQ";

  Serial.print("MQTT conectando...");

  if (mqtt.connect(clientId.c_str())) {
    Serial.println("OK");
  } else {
    Serial.print("FALLO: ");
    Serial.println(mqtt.state());
  }
}

// ==========================
// Publish
// ==========================
void publishSample(const Sample &s) {

  if (!mqtt.connected()) return;

  String msg;
  msg.reserve(120);

  msg += "{";
  msg += "\"id\":"; msg += s.id;
  msg += ",\"x\":"; msg += s.x;
  msg += ",\"y\":"; msg += s.y;
  msg += ",\"z\":"; msg += s.z;
  msg += "}";

  if (!mqtt.publish(mqtt_topic, msg.c_str())) {
    Serial.println("Publish fallido");
  }
}

// ==========================
// Web UI
// ==========================
void handleRoot() {

  String page;
  page.reserve(4096);

  page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta http-equiv="refresh" content="1">
    <title>ESP32 DAQ</title>
  </head>
  <body>
    <h2>ESP32 DAQ</h2>
  )rawliteral";
  page += "<p><strong>WiFi:</strong> ";
  page += (WiFi.status() == WL_CONNECTED) ? "Conectado" : "Desconectado";
  page += "</p>";

  page += "<p><strong>MQTT:</strong> ";
  page += mqtt.connected() ? "Conectado" : "Desconectado";
  page += "</p>";

  page += "<p><strong>Muestras en total:</strong> " + String(sampleID) + "</p>";

  page += R"rawliteral(
  <table border="1" cellpadding="5" cellspacing="0">
    <tr>
      <th>ID</th>
      <th>X</th>
      <th>Y</th>
      <th>Z</th>
    </tr>
  )rawliteral";

  for (int i = 0; i < 30; i++) {
    int idx = (writeIndex - i - 1 + BUFFER_SIZE) % BUFFER_SIZE;

    page += "<tr>";
    page += "<td>" + String(buffer[idx].id) + "</td>";
    page += "<td>" + String(buffer[idx].x) + "</td>";
    page += "<td>" + String(buffer[idx].y) + "</td>";
    page += "<td>" + String(buffer[idx].z) + "</td>";
    page += "</tr>";
  }

  page += R"rawliteral(
  </table>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

// ==========================
// Status LEDs
// ==========================
void updateStatusLEDs() {

  bool staOK  = (WiFi.status() == WL_CONNECTED);
  bool apOK   = (WiFi.getMode() & WIFI_AP) && (WiFi.softAPIP() != IPAddress(0, 0, 0, 0));
  bool mqttOK = mqtt.connected();

  digitalWrite(STAT_LED_GREEN, LOW);
  digitalWrite(STAT_LED_YELLOW, LOW);
  digitalWrite(STAT_LED_RED, LOW);

  if (!staOK) {
    // No connection to the configured Wi-Fi network
    digitalWrite(STAT_LED_RED, HIGH);

  } else if (!apOK || !mqttOK) {
    // Wi-Fi OK, but AP or MQTT has a problem
    digitalWrite(STAT_LED_YELLOW, HIGH);

  } else {
    // Everything is operating normally
    digitalWrite(STAT_LED_GREEN, HIGH);
  }
}

// ==========================
// Setup
// ==========================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(STAT_LED_GREEN, OUTPUT);
  pinMode(STAT_LED_YELLOW, OUTPUT);
  pinMode(STAT_LED_RED, OUTPUT);

  digitalWrite(STAT_LED_GREEN, LOW);
  digitalWrite(STAT_LED_YELLOW, LOW);
  digitalWrite(STAT_LED_RED, LOW);

  startWiFi();

  mqtt.setServer(mqtt_server, mqtt_port);

  server.on("/", handleRoot);
  server.begin();
}

// ==========================
// Loop
// ==========================
void loop() {

  server.handleClient();
  mqtt.loop();
  ensureMQTT();

  updateStatusLEDs();

  unsigned long now = micros();

  if (now - lastSample >= sampleInterval) {
    lastSample += sampleInterval;

    Sample s;
    s.id = sampleID++;
    s.x = analogRead(pinX);
    s.y = analogRead(pinY);
    s.z = analogRead(pinZ);

    buffer[writeIndex] = s;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    publishSample(s);
  }
}