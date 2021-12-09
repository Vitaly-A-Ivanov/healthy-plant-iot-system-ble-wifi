#include <Arduino.h>
#include <DHT.h>
#include <BH1750FVI.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>


#include <WiFi.h>
#include "Ubidots.h"

/****** BLE fields *********************************/
BLECharacteristic* moistureCharacteristic;
BLECharacteristic* tempCharacteristic;
BLECharacteristic* humdCharacteristic;
BLECharacteristic* lightCharacteristic;

// moisture service
BLEUUID soilMoistureServiceID("2d566796-47df-11ec-81d3-0242ac130003");
// temperature service
BLEUUID tempServiceID("2d5669d0-47df-11ec-81d3-0242ac130003");
// humidity service
BLEUUID humdServiceID("2d566ac0-47df-11ec-81d3-0242ac130003");
// light intensity service
BLEUUID lightServiceID("2d566b7e-47df-11ec-81d3-0242ac130003");
// moisture characterictic
BLEUUID moistureCharID("2d566c3c-47df-11ec-81d3-0242ac130003");
// moisture minimum threshold characterictic
BLEUUID moistureMinCharID("2d566ef8-47df-11ec-81d3-0242ac130003");
// moisture maximum threshold characterictic  !!
BLEUUID moistureMaxCharID("2d566fc0-47df-11ec-81d3-0242ac130003");
// temperature characterictic
BLEUUID tempCharID("2d567074-47df-11ec-81d3-0242ac130003");
// temperature minimum threshold characterictic
BLEUUID tempMinCharID("2d567128-47df-11ec-81d3-0242ac130003");
// temperature maximum threshold characterictic
BLEUUID tempMaxCharID("2d5671dc-47df-11ec-81d3-0242ac130003");
// humidity characterictic
BLEUUID humidCharID("2d567290-47df-11ec-81d3-0242ac130003");
// humidity minimum threshold characterictic
BLEUUID humidMinCharID("2d56733a-47df-11ec-81d3-0242ac130003");
// humidity maximum threshold characterictic
BLEUUID humidMaxCharID("2d5673e4-47df-11ec-81d3-0242ac130003");
// light characterictic
BLEUUID lightCharID("2d5675ec-47df-11ec-81d3-0242ac130003");
// light minimum threshold characterictic
BLEUUID lightMinCharID("2d5676b4-47df-11ec-81d3-0242ac130003");
// light maximum threshold characterictic
BLEUUID lightMaxCharID("2d56775e-47df-11ec-81d3-0242ac130003");

// BLE server
BLEServer* pServer = NULL;
/****** BLE fields *****************************************/

/****** WiFi and UBidots fields *****************************************/
const char* WIFI_SSID = "Three_5G";
const char* WIFI_PASSWORD = "Varvara2010";
const char* UBIDOTS_TOKEN = "BBFF-zFr5hYpYnLlKINl15kl1kALvXonnfs";
Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);
/****** WiFi and UBidots fields *****************************************/

/****** IFTTT fields *********************************/
const char* apiKey = "ugOtpsDjvzLpaMRaeNZPz";
const char* host = "maker.ifttt.com";

const int httpsPort = 80;
/****** IFTTT fields *********************************/

/****** moisture sensor fields *********************************/
//pin
const uint8_t MOISTURE_PIN = A0;
// last moisture level
RTC_DATA_ATTR uint16_t moistureLevel = 0;
// threshold
RTC_DATA_ATTR uint16_t minMoist = 0;
RTC_DATA_ATTR uint16_t maxMoist = 50000;

RTC_DATA_ATTR uint8_t isMoistTooHigh = false;
RTC_DATA_ATTR uint8_t isMoistTooLow = false;
RTC_DATA_ATTR uint8_t isMoistSensorFault = false;
/****** moisture sensor fields ******/

/****** temperature and humidity sensor fields *********************************/
const uint8_t DHTPIN = 4;
const uint8_t DHTTYPE = DHT11;
// last temperature level
RTC_DATA_ATTR int temperatureLevel = 0;
// threshold
RTC_DATA_ATTR int minTemp = 5000;
RTC_DATA_ATTR int maxTemp = 50000;
RTC_DATA_ATTR uint8_t isTempTooHigh = false;
RTC_DATA_ATTR uint8_t isTempTooLow = false;
RTC_DATA_ATTR uint8_t isTempSensorFault = false;

// last humidity level
RTC_DATA_ATTR uint16_t humidityLevel = 0;
// threshold
RTC_DATA_ATTR uint16_t minHumid = 0;
RTC_DATA_ATTR uint16_t maxHumid = 50000;
RTC_DATA_ATTR uint8_t isHumidTooHigh = false;
RTC_DATA_ATTR uint8_t isHumidTooLow = false;
RTC_DATA_ATTR uint8_t isHumidSensorFault = false;
DHT dht(DHTPIN, DHTTYPE);
/****** temperature and humidity sensor fields *********************************/

/****** light intensity sensor fields *********************************/
// last light intensity level
RTC_DATA_ATTR uint16_t lightLevel = 0;
// threshold
RTC_DATA_ATTR uint16_t minLight = 0;
RTC_DATA_ATTR uint16_t maxLight = 200;
RTC_DATA_ATTR uint8_t isLightTooHigh = false;
RTC_DATA_ATTR uint8_t isLightTooLow = false;
RTC_DATA_ATTR uint8_t isLightSensorFault = false;
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
/****** light intensity sensor fields *********************************/

/****** Server callback fields *********************************/
RTC_DATA_ATTR uint8_t deviceConnected = false;
RTC_DATA_ATTR uint8_t oldDeviceConnected = false;
/****** Server callback fields *********************************/

/****** Deep Sleep fields *********************************/
const uint8_t DEEP_SLEEP_TIME = 120; // in minutes
const uint8_t SLEEP_DELAY_TIME = 2; // in minutes
/****** Deep Sleep fields *********************************/

/****** Awake fields *********************************/
const uint8_t AWAKE_INTERVAL = 1; // in minutes
uint32_t previousTime = 0;
/****** Awake fields *********************************/


/*
* Checks a plant moisture.
* Returns a value from the moisture sensor.
*/
uint16_t getMoisture() {
  uint16_t ml = analogRead(MOISTURE_PIN);
  return ml;
}

/*
* Checks the enviroment temperature around the plant.
* Returns a temperature value as Celsius from the DHT sensor.
*/
int getTemperature() {
  int tp = dht.readTemperature();
  delay(500);
  return tp;
}

/*
* Checks the enviroment humidity around the plant.
* Returns a humidity value from the DHT sensor.
*/
uint16_t getHumidity() {
  uint16_t hm = dht.readHumidity();
  delay(500);
  return hm;
}

/*
* Checks the enviroment light intensity around the plant.
* Returns a light intensity as Lux value from the light intensity sensor.
*/
uint16_t getLightIntensity() {
  uint16_t lux = LightSensor.GetLightIntensity();
  return lux;
}

/*
* Check if the string consist of the numbers only.
* Returns true or false respectivly
* @param String to check
*/
bool is_number(const std::string& s) {
  return !s.empty() && std::find_if(s.begin(),
    s.end(), [] (unsigned char c) { return !std::isdigit(c); }) == s.end();
}
/*
* Checks if any of the sensors measurements are above the their threshold.
* Helps with switching the LED on or off.
*/
uint8_t checkIfProblem() {
  if (isMoistTooHigh || isMoistTooLow || isTempTooHigh || isTempTooLow
    || isHumidTooHigh || isHumidTooLow || isLightTooHigh || isLightTooLow) {
    return true;
  }
  else
    return false;
}
/*
* Sends notifications via BLE to the client if any of the threshold are above the limit.
* Checks if device connected or not to the server.
*/
void sendBLENotification() {
  if (deviceConnected) {
    //checks moisture
    if (isMoistTooLow) {
      moistureCharacteristic->setValue("Time to water your plant!");
      moistureCharacteristic->notify();
    }
    if (isMoistTooHigh) {
      moistureCharacteristic->setValue("Too much water in your plant!");
      moistureCharacteristic->notify();
    }
    if (isMoistSensorFault) {
      moistureCharacteristic->setValue("Problem with the moisture senor!");
      moistureCharacteristic->notify();
    }
    delay(100);

    // checks temperature
    if (isTempTooHigh) {
      tempCharacteristic->setValue("Too much light for your plant!");
      tempCharacteristic->notify();
    }
    if (isTempTooLow) {
      tempCharacteristic->setValue("Not enough light in your plant!");
      tempCharacteristic->notify();
    }

    if (isTempSensorFault) {
      tempCharacteristic->setValue("Problem with the light senor!");
      tempCharacteristic->notify();
    }
    delay(100);

    // checks humidity
    if (isHumidTooHigh) {
      humdCharacteristic->setValue("Humidity is very high!");
      humdCharacteristic->notify();
    }
    if (isHumidTooLow) {
      humdCharacteristic->setValue("Humidity is low!");
      humdCharacteristic->notify();
    }
    if (isHumidSensorFault) {
      humdCharacteristic->setValue("Problem with the humidity sensor!");
      humdCharacteristic->notify();
    }
    delay(1000);

    // checks light inensity
    if (isLightTooHigh) {
      lightCharacteristic->setValue("Too much light!");
      lightCharacteristic->notify();
    }
    if (isLightTooLow) {
      lightCharacteristic->setValue("Light level is ok!");
      lightCharacteristic->notify();
    }
    if (isLightSensorFault) {
      lightCharacteristic->setValue("Problem with the light inensity senor!");
      lightCharacteristic->notify();
    }
    delay(100);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}


/*
* Sends notifications via BLE to the client if any of the threshold are above the limit.
* Checks if device connected or not to the server.
*/
void sendToUbidots(int t, uint16_t m, uint16_t h, uint16_t l) {
  ubidots.add("Temperature", t);
  ubidots.add("Humidity", h);
  ubidots.add("Soil Moisture", m);
  ubidots.add("Light Intensity", l);

  bool bufferSent = false;
  bufferSent = ubidots.send();  // Will send data to a device label that matches the device Id

  if (bufferSent) {
    // Do something if values were sent properly
    Serial.println("Values sent by the device");
  }
  else {
    Serial.println("Values did not send by the device");
  }
}


void sendEmailNotification() {
  WiFiClient client;
  Serial.println("");
  Serial.print("Sending notification to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  } else {
    Serial.println("Connected to ifttt!");
    }
if(isTempTooLow) {
String url = "/trigger/temperature/with/key/ugOtpsDjvzLpaMRaeNZPz";

String temp = String(temperatureLevel);
String celsius = " °C";
String message = " - Temperature is to low!";
String v1 = temp + celsius + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}
delay(500);

if(isTempTooHigh) {
String url = "/trigger/temperature/with/key/ugOtpsDjvzLpaMRaeNZPz";

String temp = String(temperatureLevel);
String celsius = " °C";
String message = " - Temperature is to high!";
String v1 = temp + celsius + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}
delay(500);
if(isHumidTooHigh) {
String url = "/trigger/humidity/with/key/ugOtpsDjvzLpaMRaeNZPz";

String humid = String(humidityLevel);
String percentage = " %";
String message = " - Humidity is to high!";
String v1 = humid + percentage + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}
delay(500);

if(isHumidTooLow) {
String url = "/trigger/humidity/with/key/ugOtpsDjvzLpaMRaeNZPz";

String humid = String(humidityLevel);
String percentage = " %";
String message = " - Humidity is to low!";
String v1 = humid + percentage + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}

delay(500);
if(isMoistTooLow) {
String url = "/trigger/moisture/with/key/ugOtpsDjvzLpaMRaeNZPz";

String moist = String(moistureLevel);
String message = " - Moisture is to low!";
String v1 = moist + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}
delay(500);
if(isMoistTooHigh) {
String url = "/trigger/moisture/with/key/ugOtpsDjvzLpaMRaeNZPz";

String moist = String(moistureLevel);
String message = " - Moisture is to low!";
String v1 = moist + message;
Serial.println("message: " + v1);
String df1 = "{\"value1\":";
String IFTTT_POST_DATA = df1 + "\"" + v1 + "\""  + "}" ;
String IFTTT_POST_DATA_SIZE = String(IFTTT_POST_DATA.length());

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: BuildFailureDetectorESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length:" + IFTTT_POST_DATA_SIZE + "\r\n" +
               "\r\n" +
               IFTTT_POST_DATA + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("email sent");
}
delay(500);
}



/*
* Sends the unit to the deep sleep.
* Provides possibility to set the time to awake from sleep.
*/
void goToDeepSleep() {
  Serial.println("Going to sleep now for " + String(DEEP_SLEEP_TIME) + " Minutes");
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 600000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
  Serial.println("Slepping.....");
  delay(1000);

  esp_deep_sleep_start();
}
/*
* Method to print the reason by which ESP32
* has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}


void printReadingsToMonitor(int t, uint16_t m, uint16_t h, uint16_t l) {
  // moisture
  Serial.println(F(""));
  uint16_t moisture = m;
  if (isnan(moisture)) Serial.println(F("Failed to read from moisture sensor!"));
  else {
    moistureLevel = moisture;
    Serial.print(moisture);
    //checks against it's thresholds
    if (moisture < minMoist) Serial.println(F(" - Time to water your plant!"));
    if (moisture > maxMoist) Serial.println(F(" - Too much water in your plant!"));
    if (moisture >= minMoist && moisture <= maxMoist) Serial.println(F(" - Doesn't need watering!"));
  }

  //temperature
  int temp = t;
  if (isnan(temp)) Serial.println(F("Failed to read a temperature!"));
  else {
    temperatureLevel = temp;
    Serial.print(temp);
    Serial.print(F(" °C"));
    //checks against it's thresholds
    if (temp < minTemp) Serial.println(F(" - Temperature is to low!"));
    if (temp > maxTemp) Serial.println(F(" - Temperature is too high!"));
    if (temp >= minTemp && temp <= maxTemp) Serial.println(F(" - Temperature is ok!"));
  }

  // humidity
  uint16_t humid = h;
  if (isnan(humid)) Serial.println(F("Failed to read a humidity!"));
  else {
    humidityLevel = humid;
    Serial.print(humid);
    Serial.print(F(" %"));
    //checks against it's thresholds
    if (humid > maxHumid) Serial.println(F(" - Humidity is to high!"));
    if (humid < minHumid) Serial.println(F(" - Humidity is too low!"));
    if (humid >= minHumid && humid <= maxHumid) Serial.println(F(" - Humidity is ok!"));
  }

  // light intensity
  uint16_t light = l;
  if (isnan(light))  Serial.println(F("Failed to read a humidity!"));
  else {
    lightLevel = light;
    Serial.print(light);
    //checks against it's thresholds
    if (light > maxLight) Serial.println(F(" - Light is to high!"));
    if (light < minLight) Serial.println(F(" - Light is to low!"));
    if (light <= maxLight && light >= minLight) Serial.println(F(" - Light is ok!"));
  }
}

void checkAgainstThresholds(int t, uint16_t m, uint16_t h, uint16_t l) {
  // moisture
  if (isnan(m)) {
    isMoistSensorFault = true;
  }
  else {
    m < minMoist ? isMoistTooLow = true : isMoistTooLow = false;
    m > maxMoist ? isMoistTooLow = true : isMoistTooHigh = false;
    if (m >= minMoist && m <= maxMoist) {
      isMoistTooLow = false;
      isMoistTooHigh = false;
    }
  }
  //  temperature
  if (isnan(t)) {
    isTempSensorFault = true;
  }
  else {
    t < minTemp ? isTempTooLow = true : isTempTooLow = false;
    t > maxTemp ? isTempTooHigh = true : isTempTooHigh = false;
    if (t >= minTemp && t <= maxTemp) {
      isTempTooLow = false;
      isTempTooHigh = false;
    }
  }

  // humidity
  if (isnan(h)) isHumidSensorFault = true;
  else {
    h > maxHumid ? isHumidTooHigh = true : isHumidTooHigh = false;
    h < minHumid ? isHumidTooLow = true : isHumidTooLow = false;
    if (h >= minHumid && h <= maxHumid) {
      isHumidTooLow = false;
      isHumidTooHigh = false;
    }
  }

  // light intensity
  if (isnan(l)) isLightSensorFault = true;
  else {
    l > maxLight ? isLightTooHigh = true : isLightTooHigh = false;
    l < minLight ? isLightTooLow = true : isLightTooLow = false;
    if (l <= maxLight && l >= minLight) {
      isLightTooLow = false;
      isLightTooHigh = false;
    }
  }

}

/*
* The callback function that handles receiving data being sent from the client(phone) and Bluetooth connection status.
*/
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

/*
* A class definition for handling characterictics callbacks
*/
class MyCallbacks : public BLECharacteristicCallbacks {

  //this method will be call to perform writes to characteristic
  void onWrite(BLECharacteristic* pCharacteristic) {
    // moisture minimum threshold
    if (moistureMinCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        minMoist = i;
        //        int *data = getAllSensorsValues();


        if (checkIfProblem()) {
          sendBLENotification();
        }

      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // moisture maximum threshold
    if (moistureMaxCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        maxMoist = i;
        //        int *data = getAllSensorsValues();

        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // temperature minimum threshold
    if (tempMinCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        int i = atoi((pCharacteristic->getValue().c_str()));
        minTemp = i;
        int ml = getTemperature();
        if (ml < minTemp) {
          isTempTooLow = true;
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // temperature maximum threshold
    if (tempMaxCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        int i = atoi((pCharacteristic->getValue().c_str()));
        maxTemp = i;
        //         int *data = getAllSensorsValues();
        delay(500);

        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // humidity minimum threshold
    if (humidMinCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        minHumid = i;
        //        int *data = getAllSensorsValues();
        delay(500);

        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // humidity maximum threshold
    if (humidMaxCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        maxHumid = i;
        //       int *data = getAllSensorsValues();
        delay(500);

        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // light intensity minimum threshold
    if (lightMinCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        minLight = i;
        //        int *data = getAllSensorsValues();
        //        sendToUbidots(data);
        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
    // light intensity maximum threshold
    if (lightMaxCharID.equals(pCharacteristic->getUUID())) {
      if (is_number(pCharacteristic->getValue().c_str())) {
        uint16_t i = atoi((pCharacteristic->getValue().c_str()));
        maxLight = i;
        //         int *data = getAllSensorsValues();

        if (checkIfProblem()) {
          sendBLENotification();
        }
      }
      else {
        Serial.println(F("not a number"));
      }
    }
  }



  //this method will be call to perform read characteristics
  void onRead(BLECharacteristic* pCharacteristic) {
    // moisture from the sensor
    if (moistureCharID.equals(pCharacteristic->getUUID())) {
      uint16_t m = getMoisture();
      char str[8];
      sprintf(str, "%u", m);
      pCharacteristic->setValue("Current Moisture level: " + std::string(str));
    }
    // moisture minimum threshold
    if (moistureMinCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", minMoist);
      pCharacteristic->setValue("Minimum moisture level: " + std::string(str));
    }
    // moisture maximum threshold
    if (moistureMaxCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", maxMoist);
      pCharacteristic->setValue("Maximum moisture level: " + std::string(str));
    }

    // temperature from the sensor
    if (tempCharID.equals(pCharacteristic->getUUID())) {
      delay(100);
      int t = getTemperature();
      char str[8];
      sprintf(str, "%d", t);
      pCharacteristic->setValue("Current Temperature level: " + std::string(str));
    }
    // temperature maximum threshold
    if (tempMaxCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%d", maxTemp);
      pCharacteristic->setValue("Maximum temperature level: " + std::string(str));
    }
    // temperature minimum threshold
    if (tempMinCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%d", minTemp);
      pCharacteristic->setValue("Minimum temperature level: " + std::string(str));
    }
    // humdity from the sensor
    if (humidCharID.equals(pCharacteristic->getUUID())) {
      delay(100);
      uint16_t h = dht.readHumidity();
      char str[8];
      sprintf(str, "%u", h);
      pCharacteristic->setValue("Current Humidity level: " + std::string(str));

    }
    // humdity maximum threshold
    if (humidMaxCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", maxHumid);
      pCharacteristic->setValue("Maximum Humidity level " + std::string(str));
    }
    // humdity minimum threshold
    if (humidMinCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", minHumid);
      pCharacteristic->setValue("Minimum Humidity level: " + std::string(str));
    }

    // light intensity from the sensor
    if (lightCharID.equals(pCharacteristic->getUUID())) {
      uint16_t lux = getLightIntensity();
      char str[8];
      sprintf(str, "%u", lux);
      pCharacteristic->setValue("Current Light Intensity level: " + std::string(str));
    }

    // light intensity minimum threshold
    if (lightMinCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", minLight);
      pCharacteristic->setValue("Minimum Light Intensity level: " + std::string(str));
    }
    //light intensity maximum threshold
    if (lightMaxCharID.equals(pCharacteristic->getUUID())) {
      char str[8];
      sprintf(str, "%u", maxLight);
      pCharacteristic->setValue("Maximum Light Intensity level: " + std::string(str));
    }
  }
};

//Create a callback handler
MyCallbacks cb;

void setup() {

  Serial.begin(112500);

  // moisture sensor setup //
  pinMode(MOISTURE_PIN, INPUT);

  pinMode(5, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Ubibot and Wifi setup
  ubidots.wifiConnect(WIFI_SSID, WIFI_PASSWORD);
  //  ubidots.setDebug(true);

  //IFTTT setup
  
    // temperature and humidity sensor setup //
  dht.begin();

  // Light sensor setup //
  LightSensor.begin();

  // BLE Setup
  BLEDevice::init("ESP32 Feather");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // pServer->getAdvertising()->start();

  // create moisture service
  BLEService* soilMoistureService = pServer->createService(soilMoistureServiceID);

  // create temperature service
  BLEService* temperatureService = pServer->createService(tempServiceID);

  // create humidity service
  BLEService* humidityService = pServer->createService(humdServiceID);

  // create light intensity service
  BLEService* lightService = pServer->createService(lightServiceID);

  // moisture characteristic to read current value from the moisture sensor
  moistureCharacteristic = soilMoistureService->createCharacteristic(
    moistureCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  moistureCharacteristic->setCallbacks(&cb);
  moistureCharacteristic->addDescriptor(new BLE2902());

  // moisture characteristic to read and write minimum threshold value for the moisture sensor
  BLECharacteristic* moistureMinCharacteristic = soilMoistureService->createCharacteristic(
    moistureMinCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // moistureThrCharacteristic->setValue(moistureThreshold);
  moistureMinCharacteristic->setCallbacks(&cb);

  // moisture characteristic to read and write maximum threshold value for the moisture sensor
  BLECharacteristic* moistureMaxCharacteristic = soilMoistureService->createCharacteristic(
    moistureMaxCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // moistureThrCharacteristic->setValue(moistureThreshold);
  moistureMaxCharacteristic->setCallbacks(&cb);


  // temperature characteristic to read current value from the temperature sensor
  tempCharacteristic = temperatureService->createCharacteristic(
    tempCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  tempCharacteristic->setCallbacks(&cb);
  tempCharacteristic->addDescriptor(new BLE2902());

  // temperature characteristic to read and write minimum threshold value for the temperature sensor
  BLECharacteristic* tempMinCharacteristic = temperatureService->createCharacteristic(
    tempMinCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // tempThrCharacteristic->setValue(tempThreshold);
  tempMinCharacteristic->setCallbacks(&cb);

  // temperature characteristic to read and write maximum threshold value for the temperature sensor
  BLECharacteristic* tempMaxCharacteristic = temperatureService->createCharacteristic(
    tempMaxCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // tempThrCharacteristic->setValue(tempThreshold);
  tempMaxCharacteristic->setCallbacks(&cb);

  // humidity characteristic to read current value from the humidity sensor
  humdCharacteristic = humidityService->createCharacteristic(
    humidCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  humdCharacteristic->setCallbacks(&cb);
  humdCharacteristic->addDescriptor(new BLE2902());

  // humidity characteristic to read and write minimum threshold value for the humidity sensor
  BLECharacteristic* humidMinCharacteristic = humidityService->createCharacteristic(
    humidMinCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // humdThrCharacteristic->setValue(humdThreshold);
  humidMinCharacteristic->setCallbacks(&cb);

  // humidity characteristic to read and write maximum threshold value for the humidity sensor
  BLECharacteristic* humidMaxCharacteristic = humidityService->createCharacteristic(
    humidMaxCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // humdThrCharacteristic->setValue(humdThreshold);
  humidMaxCharacteristic->setCallbacks(&cb);

  // light intensity characteristic to read current value from the light intensity sensor
  lightCharacteristic = lightService->createCharacteristic(
    lightCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  lightCharacteristic->setCallbacks(&cb);
  lightCharacteristic->addDescriptor(new BLE2902());

  // humidity characteristic to read and write minimum threshold value for the humidity sensor
  BLECharacteristic* lightMinCharacteristic = lightService->createCharacteristic(
    lightMinCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // lightThrCharacteristic->setValue(lightIntensityThreshold);
  lightMinCharacteristic->setCallbacks(&cb);

  // humidity characteristic to read and write maximum threshold value for the humidity sensor
  BLECharacteristic* lightMaxCharacteristic = lightService->createCharacteristic(
    lightMaxCharID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // lightThrCharacteristic->setValue(lightIntensityThreshold);
  lightMaxCharacteristic->setCallbacks(&cb);

  //Starts all services
  temperatureService->start();
  humidityService->start();
  lightService->start();
  soilMoistureService->start();

  // Advertising config
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(tempServiceID);
  pAdvertising->addServiceUUID(humdServiceID);
  pAdvertising->addServiceUUID(lightServiceID);
  pAdvertising->addServiceUUID(soilMoistureServiceID);

  pAdvertising->setScanResponse(true);

  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  //Start advertising the device
  BLEDevice::startAdvertising();


  //  print_wakeup_reason();
}

void loop() {
  uint8_t sleepAbandonned = false;
  uint32_t currentTime = millis();
  digitalWrite(5, HIGH);
  int temp = getTemperature();
  uint16_t moist = getMoisture();
  uint16_t humid = getHumidity();
  uint16_t lux = getLightIntensity();
  printReadingsToMonitor(temp, moist, humid, lux);
  checkAgainstThresholds(temp, moist, humid, lux);
  sendToUbidots(temp, moist, humid, lux);
  if (checkIfProblem()) {
    digitalWrite(LED_BUILTIN, HIGH);
    sendBLENotification();
    sendEmailNotification();
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F(""));
    Serial.print(F("Everything is ok. Going to sleep in a "));
    Serial.print(SLEEP_DELAY_TIME);
    Serial.println(F(" minutes"));

    while (currentTime - previousTime <= SLEEP_DELAY_TIME * 60000) {
      currentTime = millis();
    }
    previousTime = currentTime;

    Serial.println(F(""));
    Serial.println(F("Reading all sensors again before sleep..."));
    int t = getTemperature();
    uint16_t m = getMoisture();
    uint16_t h = getHumidity();
    uint16_t l = getLightIntensity();
    printReadingsToMonitor(t, m, h, l);
    checkAgainstThresholds(t, m, h, l);
    sendToUbidots(t, m, h, l);
    if (checkIfProblem()) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println(F(""));
      Serial.println(F("Sleep abandoned! Needs your attention!"));
      sendBLENotification();
      sendEmailNotification();
    }
    else {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println(F(""));
      Serial.println(F("No changes..."));
      digitalWrite(5, LOW);
      goToDeepSleep();
    }
  }

  Serial.println(F(""));
  Serial.print(F("Staying awake for "));
  Serial.print(AWAKE_INTERVAL);
  Serial.println(F(" minutes"));

  while (currentTime - previousTime <= AWAKE_INTERVAL * 60000) {
    currentTime = millis();
    if (!checkIfProblem()) {
      break;
    }
  }
  previousTime = currentTime;
  delay(1000);
}
