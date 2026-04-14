#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// ===== WIFI =====
const char* ssid = "XXXXXX"; non wifi
const char* password = "XXXXXXX"; MDP

// ===== MQTT =====
const char* mqtt_server = "IP DE TON SERVEUR";
const char* mqtt_user = "NON DE TON MQTT";
const char* mqtt_password = "TON MDP";

WiFiClient espClient;
PubSubClient client(espClient);

// ===== SERIAL MEGA =====
SoftwareSerial megaSerial(D5, D6);

// ===== BUFFER SERIAL =====
String serialBuffer = "";

// ===== DERNIERES VALEURS =====
float lastTemp = -999;
float lastPh = -999;
float lastRedox = -999;
int lastN1 = -1;
int lastN2 = -1;

unsigned long lastPublish = 0;

// ===== FILTRE MOYENNE =====
#define SMOOTH_COUNT 5

float tempBuffer[SMOOTH_COUNT];
float phBuffer[SMOOTH_COUNT];
float redoxBuffer[SMOOTH_COUNT];

int bufferIndex = 0;
bool bufferFilled = false;

// ===== WIFI =====
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");
}

// ===== MQTT =====
void connectMQTT() {

  while (!client.connected()) {

    Serial.print("Connexion MQTT...");

    if (client.connect(
          "ESP8266Client",
          mqtt_user,
          mqtt_password,
          "aquarium/status",
          0,
          true,
          "offline")) {

      Serial.println("OK");
      client.publish("aquarium/status", "online", true);

    } else {
      Serial.print("ECHEC rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ===== MOYENNE =====
float average(float *buffer) {
  float sum = 0;
  for (int i = 0; i < SMOOTH_COUNT; i++) {
    sum += buffer[i];
  }
  return sum / SMOOTH_COUNT;
}

// ===== TRAITEMENT =====
void processData(String data) {

  data.trim();

  float temp, ph, redox;
  int n1, n2;

  if (sscanf(data.c_str(), "%f;%f;%f;%d;%d",
             &temp, &ph, &redox, &n1, &n2) == 5) {

    // ===== VALIDATION =====
    if (temp < 0 || temp > 50) return;
    if (ph < 0 || ph > 14) return;
    if (redox < -1000 || redox > 1000) return;

    // ===== BUFFER =====
    tempBuffer[bufferIndex] = temp;
    phBuffer[bufferIndex] = ph;
    redoxBuffer[bufferIndex] = redox;

    bufferIndex++;

    if (bufferIndex >= SMOOTH_COUNT) {
      bufferIndex = 0;
      bufferFilled = true;
    }

    if (!bufferFilled) return;

    // ===== MOYENNE =====
    float tempAvg = average(tempBuffer);
    float phAvg = average(phBuffer);
    float redoxAvg = average(redoxBuffer);

    bool changed = false;

    // ===== ANTI BRUIT =====
    if (abs(tempAvg - lastTemp) > 0.2) changed = true;
    if (abs(phAvg - lastPh) > 0.03) changed = true;
    if (abs(redoxAvg - lastRedox) > 😎 changed = true;
    if (n1 != lastN1 || n2 != lastN2) changed = true;

    // ===== TIMER 30s =====
    bool timeout = (millis() - lastPublish > 30000);

    if (changed || timeout) {

      StaticJsonDocument<128> doc;

      doc["temp"] = tempAvg;
      doc["ph"] = phAvg;
      doc["redox"] = redoxAvg;
      doc["n1"] = n1;
      doc["n2"] = n2;

      char buffer[128];
      serializeJson(doc, buffer);

      Serial.println("MQTT SEND:");
      Serial.println(buffer);

      client.publish("aquarium/data", buffer, true);

      lastTemp = tempAvg;
      lastPh = phAvg;
      lastRedox = redoxAvg;
      lastN1 = n1;
      lastN2 = n2;

      lastPublish = millis();
    }
  } else {
    Serial.println("PARSE ERROR");
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  megaSerial.begin(9600);

  setup_wifi();

  client.setServer(mqtt_server, 1883);

  Serial.println("ESP READY");
}

// ===== LOOP =====
void loop() {

  // ===== WIFI CHECK =====
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnexion WiFi...");
    setup_wifi();
  }

  // ===== MQTT CHECK =====
  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();

  // ===== LECTURE SERIAL =====
  while (megaSerial.available()) {

    char c = megaSerial.read();

    if (c == '\n') {
      processData(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}
