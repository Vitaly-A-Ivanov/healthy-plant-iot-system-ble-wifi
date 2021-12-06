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
RTC_DATA_ATTR int minTemp = 0;
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
const uint8_t SLEEP_DELAY_TIME = 15; // in minutes

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
  return tp;
}

/*
* Checks the enviroment humidity around the plant.
* Returns a humidity value from the DHT sensor.
*/
uint16_t getHumidity() {
  uint16_t hm = dht.readHumidity();
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
    digitalWrite(LED_BUILTIN, HIGH);
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
void sendToUbidots() {
  uint16_t humid = humidityLevel;
  int temp = temperatureLevel;
  uint16_t light = lightLevel;
  uint16_t moist = moistureLevel;
  


  ubidots.add("Temperature", temp); 
  ubidots.add("Humidity", humid);
  ubidots.add("Soil Moisture", moist);
  ubidots.add("Light Intensity", light);

  bool bufferSent = false;
  bufferSent = ubidots.send();  // Will send data to a device label that matches the device Id

  if (bufferSent) {
    // Do something if values were sent properly
    Serial.println("Values sent by the device");
  } else {
     Serial.println("Values did not send by the device");
    }
}
/*
* Sends the unit to the deep sleep.
* Provides possibility to set the time to awake from sleep.
*/
void goToDeepSleep() {
  Serial.println("Going to sleep now for " + String(DEEP_SLEEP_TIME) + " Minutes");
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 600000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1);
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

/*
* Reads all sensors. Compare results with their thresholds.
* Prints the message to the monitor if readings above the threshold
*/
void readSensors() {
  // read the moisture
  Serial.println(F(""));
  uint16_t moisture = getMoisture();
  delay(100);
  if (isnan(moisture)) {
    Serial.println(F("Failed to read from moisture sensor!"));
    isMoistSensorFault = true;
  }
  else {
    moistureLevel = moisture;
    Serial.print(moisture);

    //checks against it's thresholds
    if (moisture < minMoist) {
      Serial.println(F(" - Time to water your plant!"));
      isMoistTooLow = true;
    }
    else {
      isMoistTooLow = false;
    }
    if (moisture > maxMoist) {
      Serial.println(F(" - Too much water in your plant!"));
      isMoistTooLow = true;
    }
    else {
      isMoistTooHigh = false;
    }
    if (moisture >= minMoist && moisture <= maxMoist) {
      isMoistTooLow = false;
      isMoistTooHigh = false;
      Serial.println(F(" - Doesn't need watering!"));
    }
  }
  // read temperature
  int temp = getTemperature();
  delay(100);
  if (isnan(temp)) {
    Serial.println(F("Failed to read a temperature!"));
    isTempSensorFault = true;
  }
  else {
    temperatureLevel = temp;
    Serial.print(temp);
    Serial.print(F(" °C"));

    //checks against it's thresholds
    if (temp < minTemp) {
      Serial.println(F(" - Temperaure is to low!"));
      isTempTooLow = true;
    }
    else {
      isTempTooLow = false;
    }
    if (temp > maxTemp) {
      Serial.println(F(" - Temperaure is too high!"));
      isTempTooHigh = true;
    }
    else {
      isTempTooHigh = false;
    }
    if (temp >= minTemp && temp <= maxTemp) {
      isTempTooLow = false;
      isTempTooHigh = false;
      Serial.println(F(" - Temperature is ok!"));
    }
  }

  // read humidity
  uint16_t humid = getHumidity();
  if (isnan(humid)) {
    Serial.println(F("Failed to read a humidity!"));
    isHumidSensorFault = true;
  }
  else {
    humidityLevel = humid;
    Serial.print(humid);
    Serial.print(F(" %"));

    //checks against it's thresholds
    if (humid > maxHumid) {
      Serial.println(F(" - Humidity is to high!"));
      isHumidTooHigh = true;
    }
    else {
      isHumidTooHigh = false;
    }
    if (humid < minHumid) {
      Serial.println(F(" - Humidity is too low!"));
      isHumidTooLow = true;
    }
    else {
      isHumidTooLow = false;
    }
    if (humid >= minHumid && humid <= maxHumid) {
      isHumidTooLow = false;
      isHumidTooHigh = false;
      Serial.println(F(" - Humidity is ok!"));
    }
  }

  // read light intensity
  uint16_t light = getLightIntensity();
  if (isnan(light)) {
    isLightSensorFault = true;
    Serial.println(F("Failed to read a humidity!"));
  }
  else {
    lightLevel = light;
    Serial.print(light);

    //checks against it's thresholds
    if (light > maxLight) {
      isLightTooHigh = true;
      Serial.println(F(" - Light is to high!"));
    }
    if (light < minLight) {
      isLightTooLow = true;
      Serial.println(F(" - Light is to low!"));
    }
    if (light <= maxLight && light >= minLight) {
      isLightTooLow = false;
      isLightTooHigh = false;
      Serial.println(F(" - Light is ok!"));
    }
  }
}

/*
 * Initialize WiFi connection with the board.
//*/
//void initWiFi() {
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//  Serial.print("Connecting to WiFi ..");
//  while (WiFi.status() != WL_CONNECTED) {
//    Serial.print('.');
//    delay(1000);
//  }
//  Serial.println(WiFi.localIP());
//}

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
        readSensors();

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
                readSensors();

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
                readSensors();

        if (checkIfProblem()) {
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
                readSensors();

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
                readSensors();

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
                readSensors();

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
                readSensors();
        sendToUbidots();
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
                readSensors();

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

//   WiFi setup
//   initWiFi();
//   Serial.print("RRSI: ");
//   Serial.println(WiFi.RSSI());

// Ubibot setup

  ubidots.wifiConnect(WIFI_SSID, WIFI_PASSWORD);
//  ubidots.setDebug(true);


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


  print_wakeup_reason();
}

void loop() {
  digitalWrite(5, HIGH);
  readSensors();
  sendToUbidots();
  if (checkIfProblem()) {
    sendBLENotification();
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    Serial.println(F(""));
    Serial.print(F("Everything is ok. Going to sleep in a "));
    Serial.print(SLEEP_DELAY_TIME);
    Serial.println(F(" minutes"));
    delay(SLEEP_DELAY_TIME * 60000); 
    Serial.println(F(""));
    Serial.println(F("Reading all sensors again before sleep..."));
    readSensors();
    sendToUbidots();
    if (checkIfProblem()) {
      delay(1000);
      Serial.println(F(""));
      Serial.println(F("Sleep abandoned! Needs your attention!"));
      sendBLENotification();
    }
    else {
      delay(1000);
      Serial.println(F(""));
      Serial.println(F("No changes..."));
      digitalWrite(5, LOW);
      delay(1000);
      goToDeepSleep();
    }
  }
  delay(5000);

}
