#include <Wire.h>
#include <BME280I2C.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <time.h>


unsigned long myChannelNumber = 1234;
const char * myWriteAPIKey = "";
const char* ssid = "";
const char* password = "";


const char* server = "api.thingspeak.com";
WiFiClient client;

BME280I2C bme;

int sleepTime = 60;
unsigned long curTime;

void setup() {
   curTime = millis();
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - curTime > (60 * 1000)) {
      goToSleep();
    }
    Serial.println(millis() - curTime);
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Wire.begin();
  while (!bme.begin())
  {
    if (millis() - curTime > (60 * 1000)) {
      goToSleep();
    }
    Serial.println(millis() - curTime);
    delay(1000);
  }
  // bme.chipID(); // Deprecated. See chipModel().
  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }
  ThingSpeak.begin(client);
  measure();

  // sleep
  WiFi.disconnect(true);
  goToSleep();
}
void goToSleep() {
    Serial.print("Sleeping for: ");
    Serial.print(sleepTime);
    Serial.println(" seconds.");
    ESP.deepSleep(sleepTime * 1000000);
}
void measure() {
  float t(NAN);
  float p(NAN);
  float h(NAN);
  float volt = 0.0;
  unsigned int raw = 0;
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(p, t, h, tempUnit, presUnit);


  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" degrees Celcius Pressure: ");
  Serial.print(p);
  Serial.print(" Humidity: ");
  Serial.print(h);
  Serial.println("% send to Thingspeak");
  pinMode(A0, INPUT);
  raw = analogRead(A0);
  volt = raw / 1023.0;
  volt = volt * 4.2;
  Serial.print(volt);

 if (volt > 4.0) {
  sleepTime = 60;
 } else if (volt > 3.9) {
  sleepTime = 120;
 } else if (volt > 3.85) {
  sleepTime = 180;
 } else if (volt > 3.75) {
  sleepTime = 240;
 } else {
  sleepTime = 60 * 30;
 }

  // wifi
  float rssi = WiFi.RSSI();
  
  ThingSpeak.setField(1, t);
  ThingSpeak.setField(2, p);
  ThingSpeak.setField(3, h);
  ThingSpeak.setField(4, volt);
  ThingSpeak.setField(5, rssi);

  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

void loop() {
}
