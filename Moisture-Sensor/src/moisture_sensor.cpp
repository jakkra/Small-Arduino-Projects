/**
 * Connect to WiFi -> read moisture level -> send to server -> check for OTA -> go to deep sleep for 2 minutes -> repeat
 * Since deep sleep is used, we can't handle FOTA when sleeping, therefore I descided pulling pin 4/D2 LOW will put the esp8266 into OTA mode next boot.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ESP8266HTTPClient.h>

//#define DEBUG

#ifdef DEBUG
#define LOG(msg) Serial.print(msg)
#define LOGF Serial.printf
#else
#define LOG(msg)
#define LOGF(...)
#endif

void setupOTA();
void logMoistureLevel();

const char* ssid = "";
const char* password = "";
const char* url = "http://207.154.239.115/api/moisture";
const char* accessToken = "myBackendAccessToken";

// YL-39 + YL-69 moisture sensor
byte moistureSensorPin = A0;
byte moistureSensorVcc = 5; // D1;
byte otaPinEnable = 4; // D2;

uint8_t otaInProgress;

void setup() {
  otaInProgress = 0;
  // Init the moisture sensor board
  pinMode(moistureSensorVcc, OUTPUT);
  pinMode(otaPinEnable, INPUT_PULLUP);
  digitalWrite(moistureSensorVcc, LOW);
#ifdef DEBUG
  Serial.begin(115200);
  Serial.setTimeout(2000);
#endif
  LOG("START\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    LOG("Connection Failed! Rebooting...\n");
    delay(5000);
    ESP.restart();
  }
  setupOTA();
  logMoistureLevel();
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    LOG("Start updating\n");
    otaInProgress = 1;
  });

  ArduinoOTA.onEnd([]() {
    LOG("\nEnd\n");
    otaInProgress = 0;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    LOGF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    LOGF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) LOG("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) LOG("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) LOG("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) LOG("Receive Failed");
    else if (error == OTA_END_ERROR) LOG("End Failed");
  });

  ArduinoOTA.begin();
  LOG("IP address: ");
  LOG(WiFi.localIP());
  LOG("\n\n");
}

void checkOTA() {
  ArduinoOTA.handle();
}

int read_moisture_sensor() {
  digitalWrite(moistureSensorVcc, HIGH);
  delay(500);
  int value = analogRead(moistureSensorPin);
  digitalWrite(moistureSensorVcc, LOW);
  return  1023 - value;
}

void logMoistureLevel() {
  HTTPClient http;
  int httpCode;
  char jsonData[50];

  int analogValue = read_moisture_sensor();
  int moisture = map(analogValue, 5, 756, 0, 100);

  LOGF("Moisture Level (0-1023): %d\n", analogValue);
  LOGF("Moisture Level %d\n", moisture);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-access-token", accessToken);
  sprintf(jsonData, "{\"moisture\": %d}", moisture);
  LOG(jsonData);
  httpCode = http.POST(jsonData);
  if (httpCode > 0) {
    LOGF("\n[HTTP] POST... code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      LOG(payload);
    }
  } else {
    LOGF("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());    
  }
}

void loop() {
  if (!otaInProgress && (digitalRead(otaPinEnable) == LOW)) {
    LOG("START OTA\n");
    otaInProgress = 1;
  }
  ArduinoOTA.handle();
  if (!otaInProgress) {
    LOG("\nGoing to sleep\n");
#ifdef DEBUG
    ESP.deepSleep(10e6);
#else
    ESP.deepSleep(120e6);
#endif
  }
}


