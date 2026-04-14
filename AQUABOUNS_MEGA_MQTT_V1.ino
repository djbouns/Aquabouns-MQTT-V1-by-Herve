#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
#define PH_PIN A0
#define REDOX_PIN A1
#define NIVEAU1_PIN 3
#define NIVEAU2_PIN 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);   // Debug PC
  Serial1.begin(9600);    // vers ESP

  pinMode(NIVEAU1_PIN, INPUT_PULLUP);
  pinMode(NIVEAU2_PIN, INPUT_PULLUP);

  sensors.begin();

  Serial.println("MEGA READY");
}

void loop() {

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  int phRaw = analogRead(PH_PIN);
  float ph = phRaw * (14.0 / 1023.0);

  int redoxRaw = analogRead(REDOX_PIN);
  float redox = redoxRaw * (1000.0 / 1023.0);

  int niveau1 = digitalRead(NIVEAU1_PIN);
  int niveau2 = digitalRead(NIVEAU2_PIN);

  String data = String(temp, 2) + ";" +
                String(ph, 2) + ";" +
                String(redox, 0) + ";" +
                String(niveau1) + ";" +
                String(niveau2);

  // Debug PC
  Serial.print("DEBUG: ");
  Serial.println(data);

  // Envoi vers ESP
  Serial1.println(data);

  delay(5000);
}
