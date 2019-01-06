/*
   ESP8266 Weather Station by Wilyarti Howard (C) 2019
   github.com/wilyarti
   License BSD 2

*/

/*
    ESP8266 ThingSpeak Device

    An IoT Device that deep sleeps to save power and wakes up regularly to send in a sensor reading

 *** DEFAULT sketch that just sends connect time and Wifi RSSI to Thinkspeak, update for your sensor ***

    Uses WifiManager to configure Wifi network and ThingSpeak channel/key, so no id's or passwords
    need to be hardcoded in the sketch.

    Uses ThingSpeak channel metadata to configure the device for things like the deep sleep
    interval and URL for OTA firmware updates. To do that, in the TingSpeak channel setings
    metadata field add some json with the following optional fields:

    {
      "publishInterval" : 600,
      "firmwareVersion" : "1.2.3",
      "firmwareURL" : "http://somehost.com/myfirmware.bin"
    }

    The device updates the configuration from the metadata at power on and once per day.

    Author: Anthony Elder
    License: Apache License v2
*/
// OTA updater
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Ticker.h>

// Weather station begin
#include <Wire.h>
#include <BME280I2C.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <SparkFun_VEML6075_Arduino_Library.h>
#include <time.h>
// Weather station end
const String VERSION = "1.0.0";

#define LED_PIN BUILTIN_LED // gpio2 for ESP-12, gpio1 for ESP-01

#define METADATA_CHK_SECS 24 * 60 * 60 // Check for updated config metadata once a day 

// this is the config persisted to EEPROM so retained over power offs
typedef struct {
  String thingSpeakChannel;
  String thingSpeakKey;
  int    publishInterval;
  int    initializedFlag;
} _Config;

_Config theConfig;

// this counts the deep sleep wakeups to time when a day is up
typedef struct {
  int wakeupCounter;
  int initializedFlag;
} _rtcStore;

_rtcStore rtcStore;

#define INITIALIZED_MARKER 7216

Ticker ledBlinker;

// Weather station begin
WiFiClient client;

BME280I2C bme;

int sleepTime = 60;
unsigned long curTime;

// Weather station end

void setup() {
  Serial.begin(115200); Serial.println();
  Serial.print("Firmware version: "); Serial.println(VERSION);

  Serial.println("Loading config...");
  loadConfig();
  Serial.println("Checking wake up count...");
  doWakeupCount();

  Serial.println("Connecting to Wifi...");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // fast blink LED while Wifi manager running config AP
    ledBlinker.attach(0.2, []() {
      digitalWrite(LED_PIN, ! digitalRead(LED_PIN));
    });
    doWifiManager();
    ledBlinker.detach();
    digitalWrite(LED_PIN, HIGH); // off
  }

  if (rtcStore.wakeupCounter == 0) {
    updateConfigFromChannelMetadata();
  }

  Serial.println("Measuring...");
  Wire.begin();
  measure();

  Serial.println("Going to sleep...");
  goToSleep();
}

void loop() {
  // shouldn't get here as deepsleep'ing so everything happens in setup()
}

void readSensor() {
  // add any code to initialize and read a sensor
  // save reading in a global variable to be used by sendReading();
}

void goToSleep() {
  Serial.print("Up for "); Serial.print(millis());
  Serial.print(" milliseconds, going to sleep for "); Serial.print(theConfig.publishInterval); Serial.println(" seconds");
  float volt = 0.0;
  unsigned int raw = 0;
  pinMode(A0, INPUT);
  raw = analogRead(A0);
  volt = raw / 1023.0;
  volt = volt * 4.2;

  if (volt > 4.0) {
    sleepTime = theConfig.publishInterval;
  } else if (volt > 3.9) {
    sleepTime = 120;
  } else if (volt > 3.85) {
    sleepTime = 180;
  } else if (volt > 3.75) {
    sleepTime = 240;
  } else {
    sleepTime = 60 * 30;
  }
  Serial.print("Sleeping for: ");
  Serial.print(sleepTime);
  Serial.println(" seconds.");
  ESP.deepSleep(sleepTime * 1000000);
}

void doWakeupCount() {
  system_rtc_mem_read(65, &rtcStore, sizeof(rtcStore));

  if (rtcStore.initializedFlag != INITIALIZED_MARKER) {
    rtcStore.initializedFlag = INITIALIZED_MARKER;
    rtcStore.wakeupCounter = 0;
  } else {
    rtcStore.wakeupCounter += 1;
  }

  if (theConfig.publishInterval * rtcStore.wakeupCounter > METADATA_CHK_SECS) {
    rtcStore.wakeupCounter = 0;
  }

  system_rtc_mem_write(65, &rtcStore, sizeof(rtcStore));

  Serial.print("doWakeupCount: rtcStore.initializedFlag="); Serial.print(rtcStore.initializedFlag);
  Serial.print(", rtcStore.wakeupCounter="); Serial.println(rtcStore.wakeupCounter);
}

bool shouldSaveConfig = false;

void doWifiManager() {
  WiFiManagerParameter custom_thingspeak_channel("channel", "ThingSpeak Channel", theConfig.thingSpeakChannel.c_str(), 9);
  WiFiManagerParameter custom_thingspeak_key("key", "ThingSpeak Key", theConfig.thingSpeakKey.c_str(), 17);
  WiFiManagerParameter custom_publish_interval("publishInterval", "Publish Interval", String(theConfig.publishInterval).c_str(), 6);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback([]() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
  });

  wifiManager.addParameter(&custom_thingspeak_channel);
  wifiManager.addParameter(&custom_thingspeak_key);
  wifiManager.addParameter(&custom_publish_interval);

  wifiManager.setTimeout(240); // 4 minutes

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  String apName = String("ESP-") + ESP.getChipId();
  if (!wifiManager.autoConnect(apName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  theConfig.thingSpeakChannel = String(custom_thingspeak_channel.getValue());
  theConfig.thingSpeakKey = String(custom_thingspeak_key.getValue());
  theConfig.publishInterval = String(custom_publish_interval.getValue()).toInt();

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    saveConfig();
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void updateConfigFromChannelMetadata() {
  String url = String("http://api.thingspeak.com/channels/") + theConfig.thingSpeakChannel + "/feeds.json?metadata=true&results=0";
  Serial.print("updateConfigFromChannelMetadata: "); Serial.println(url);
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("updateConfigFromChannelMetadata httpCode: ");  Serial.println(httpCode);
  String payload = http.getString();
  http.end();
  if (payload.length() < 1) {
    return;
  }

  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.print("updateConfigFromChannelMetadata: payload parse FAILED: ");
    Serial.print("payload: "); Serial.println(payload);
    return;
  }
  Serial.print("updateConfigFromChannelMetadata: payload parse OK: "); root.prettyPrintTo(Serial); Serial.println();

  String metaDate = root["channel"]["metadata"];
  Serial.println(metaDate);

  StaticJsonBuffer<500> jsonBuffer2;
  JsonObject& md = jsonBuffer2.parseObject(metaDate);
  if (!md.success()) {
    Serial.print("updateConfigFromChannelMetadata: md parse FAILED: ");
    Serial.print("updateConfigFromChannelMetadata: md: "); Serial.println(metaDate);
    return;
  }
  Serial.print("updateConfigFromChannelMetadata: metaDate parse OK: "); md.prettyPrintTo(Serial); Serial.println();

  boolean configUpdated = false;
  if (md.containsKey("publishInterval")) {
    int pi = md["publishInterval"];
    if (pi != theConfig.publishInterval) {
      theConfig.publishInterval = pi;
      configUpdated = true;
    }
  } else {
    Serial.println("updateConfigFromChannelMetadata: no publish interval");
  }

  if (configUpdated) {
    saveConfig();
  }

  if (md.containsKey("firmwareURL")) {
    if (md.containsKey("firmwareVersion")) {
      String firmwareV = md["firmwareVersion"];
      String firmwareUrl = md["firmwareURL"];
      if (firmwareV != VERSION) {
        doFirmwareUpdate(firmwareUrl);
      }
    } else {
      Serial.println("updateConfigFromChannelMetadata: no firmwareVersion");
    }
  } else {
    Serial.println("updateConfigFromChannelMetadata: no firmwareURL");
  }
}

void saveConfig() {
  EEPROM.begin(512);
  int addr = 0;
  EEPROM.put(addr, INITIALIZED_MARKER);
  addr += sizeof(INITIALIZED_MARKER);
  addr = eepromWriteString(addr, theConfig.thingSpeakChannel);
  addr = eepromWriteString(addr, theConfig.thingSpeakKey);
  EEPROM.put(addr, theConfig.publishInterval);
  addr += sizeof(theConfig.publishInterval);

  // update loadConfig() and printConfig() if anything else added here

  EEPROM.commit();

  Serial.println("Config saved:"); printConfig();
}

void loadConfig() {
  EEPROM.begin(512);

  int addr = 0;
  EEPROM.get(addr, theConfig.initializedFlag);            addr += sizeof(theConfig.publishInterval);
  if (theConfig.initializedFlag != INITIALIZED_MARKER) {
    Serial.println("*** No config!");
    return;
  }

  theConfig.thingSpeakChannel = eepromReadString(addr);   addr += theConfig.thingSpeakChannel.length() + 1;
  theConfig.thingSpeakKey = eepromReadString(addr);       addr += theConfig.thingSpeakKey.length() + 1;
  EEPROM.get(addr, theConfig.publishInterval);            addr += sizeof(theConfig.publishInterval);

  EEPROM.commit();

  Serial.println("Config loaded: "); printConfig();
}

void printConfig() {
  Serial.print("ThingSpeak channel: '"); Serial.print(theConfig.thingSpeakChannel);
  Serial.print("', ThingSpeak Key="); Serial.print(theConfig.thingSpeakKey);
  Serial.print(", publishInterval="); Serial.print(theConfig.publishInterval);
  Serial.println();
}

// get these eeprom read/write String functions in Arduino code somewhere?
int eepromWriteString(int addr, String s) {
  int l = s.length();
  for (int i = 0; i < l; i++) {
    EEPROM.write(addr++, s.charAt(i));
  }
  EEPROM.write(addr++, 0x00);
  return addr;
}

String eepromReadString(int addr) {
  String s;
  char c;
  while ((c = EEPROM.read(addr++)) != 0x00) {
    s += c;
  }
  return s;
}

void doFirmwareUpdate(String firmwareUrl) {
  Serial.print("doFirmwareUpdate: from "); Serial.println(firmwareUrl);

  t_httpUpdate_return ret = ESPhttpUpdate.update(firmwareUrl);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.print("doFirmwareUpdate: HTTP_UPDATE_FAILD Error :"); Serial.print(ESPhttpUpdate.getLastError()); Serial.println(ESPhttpUpdate.getLastErrorString());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("doFirmwareUpdate: HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("doFirmwareUpdate: HTTP_UPDATE_OK");
      break;
  }
}

void measure() {
  curTime = millis();

  while (!bme.begin())
  {
    if (millis() - curTime > (60 * 1000)) {
      break;
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
  // wifi
  float rssi = WiFi.RSSI();
  
  String url = String("http://api.thingspeak.com/update?api_key=") + theConfig.thingSpeakKey +
               "&field1=" + t +
               "&field2=" + p +
               "&field3=" + h +
               "&field4=" + volt +
               "&field5=" + rssi;
  Serial.println(url);
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("sendReading: httpCode: ");  Serial.println(httpCode);
  http.end();
  
}
