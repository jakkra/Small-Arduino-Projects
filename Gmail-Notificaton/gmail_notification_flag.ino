#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>

const char* ssid = "";
const char* password = "";
const char* url = "http://207.154.239.115/api/user/hasUnreadMail?token=myBackendAccessToken";
Servo myservo;

boolean hasUnreadMail = false;

void setup () {
  pinMode(2, OUTPUT);

  myservo.attach(2);
  myservo.write(50);
  Serial.begin(115200);
  setUpWiFi();
}

void setUpWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Making request");

    HTTPClient http;

    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {

      String payload = http.getString();
      Serial.println(payload);
      if (payload.equals("true")){
        moveStepper(true);
      } else {
        moveStepper(false);
      }
    } else {
      Serial.println(httpCode);
      String payload = http.getString();
      Serial.println(payload);
      Serial.println(http.errorToString(httpCode).c_str());

    }

    http.end();
  }

  delay(1000); // Send a request every second
}

void moveStepper(boolean newHasUnreadMail){
  double pos;
  if(!hasUnreadMail && newHasUnreadMail) {
    hasUnreadMail = true;
    for (pos = 50; pos <= 110; pos += 1)
    {
      myservo.write(pos);
      delay(10);
    }
    myservo.write(180);
  } else if(hasUnreadMail && !newHasUnreadMail) {
    hasUnreadMail = false;
    for (pos = 180; pos >= 100; pos -= 1)
    {
      myservo.write(pos);
      delay(10); // waits 10ms for the servo to reach the position
    }
    myservo.write(50);
  }
}
