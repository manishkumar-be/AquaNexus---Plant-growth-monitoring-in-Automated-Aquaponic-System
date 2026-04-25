#include <WiFi.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === WiFi credentials ===
const char* ssid = "EACCESS_25";
const char* pass = "hostelnet";

// === ThingSpeak ===
const char* server = "api.thingspeak.com";
String apiKey = "XRDAG0APJUXQF3ML";
WiFiClient client;

// === DHT11 Setup ===
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// === Turbidity Sensor ===
#define TURBIDITY_PIN 34

// === DS18B20 Setup ===
#define ONE_WIRE_BUS 23
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);

// === TDS Sensor ===
#define TDS_PIN 35

// === LCD Setup ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16x2 display

void setup() {
  Serial.begin(115200);
  dht.begin();
  waterSensor.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, pass);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    lcd.clear();
    lcd.print("WiFi Connected");
  } else {
    Serial.println("\nWiFi failed!");
    lcd.clear();
    lcd.print("WiFi Failed");
  }

  delay(2000);
}

void loop() {
  // === Read DHT11 ===
  float humidity = dht.readHumidity();
  float airTemp = dht.readTemperature();

  if (isnan(humidity) || isnan(airTemp)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.print("DHT Error");
    delay(20000);
    return;
  }

  // === Read Turbidity ===
  int turbidityRaw = analogRead(TURBIDITY_PIN);
  float voltage = turbidityRaw * (3.3 / 4095.0);
  float turbidityNTU = -1120.4 * sq(voltage) + 5742.3 * voltage - 4352.9;

  if (turbidityNTU < 0) turbidityNTU = 0;

  // Also compute turbidity % cleanliness using mapping
  int turbidityPercent = map(turbidityRaw, 0, 640, 100, 0);
  if (turbidityPercent < 0) turbidityPercent = 0;
  if (turbidityPercent > 100) turbidityPercent = 100;

  // === Read Water Temperature from DS18B20 ===
  waterSensor.requestTemperatures();
  float waterTemp = waterSensor.getTempCByIndex(0);

  if (waterTemp == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read from DS18B20 sensor!");
    lcd.clear();
    lcd.print("Temp Sensor Err");
    delay(20000);
    return;
  }

  // === Read TDS and calculate EC ===
  int tdsRaw = analogRead(TDS_PIN);
  float tdsVoltage = tdsRaw * (3.3 / 4095.0);
  float ecValue = (tdsVoltage / 2.0) * 1000;   // EC in μS/cm
  float tdsValue = ecValue * 0.5;              // TDS in ppm

  // === Send to ThingSpeak ===
  if (client.connect(server, 80)) {
    String postStr = "api_key=" + apiKey;
    postStr += "&field1=" + String(airTemp);
    postStr += "&field2=" + String(humidity);
    postStr += "&field3=" + String(waterTemp);
    postStr += "&field4=" + String(turbidityNTU);    // Send NTU to ThingSpeak
    postStr += "&field5=" + String(tdsValue);
    postStr += "&field6=" + String(ecValue);

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postStr.length());
    client.println();
    client.print(postStr);

    Serial.println("Data sent:");
    Serial.print("Air Temp: "); Serial.print(airTemp); Serial.print(" °C, ");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.print(" %, ");
    Serial.print("Water Temp: "); Serial.print(waterTemp); Serial.print(" °C, ");
    Serial.print("Turbidity: "); Serial.print(turbidityNTU); Serial.print(" NTU, ");
    Serial.print("Turbidity: "); Serial.print(turbidityPercent); Serial.print(" % clean, ");
    Serial.print("TDS: "); Serial.print(tdsValue); Serial.print(" ppm, ");
    Serial.print("EC: "); Serial.print(ecValue); Serial.println(" μS/cm");

    // === Safe ThingSpeak response loop with timeout ===
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println("ThingSpeak Response: " + line);
        timeout = millis(); // reset timeout if data is coming
      }
    }
  } else {
    Serial.println("Failed to connect to ThingSpeak (HTTP)");
    lcd.clear();
    lcd.print("TS Conn Err");
    delay(3000);
  }

  client.stop();

// === LCD Display Pages ===

  // Page 1: Air Temp & Humidity
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Air:");
  lcd.print(airTemp, 1);
  lcd.print(" H:");
  lcd.print(humidity, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Water:");
  lcd.print(waterTemp, 1);
  lcd.print("C");
  delay(7000);

  // Page 2: Turbidity
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Turb: ");
  lcd.print(turbidityPercent);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("NTU: ");
  lcd.print(turbidityNTU, 0);
  delay(7000);

  // Page 3: TDS & EC
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TDS: ");
  lcd.print(tdsValue, 0);
  lcd.print("ppm");

  lcd.setCursor(0, 1);
  lcd.print("EC: ");
  lcd.print(ecValue, 0);
  lcd.print("uS/cm");
  delay(7000);
}


