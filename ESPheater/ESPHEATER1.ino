#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp32-hal-ledc.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
//bool oldDeviceConnected = false;


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define TEMPPIN 15
#define HEATPIN 13
//#define STATUSPIN 12
#define RESETPIN 4

#define TARGET_TEMP 98
#define OVERHEAT_TEMP 101
#define STABILIZING_MARGIN 2

#define REDPIN    25
#define GREENPIN  26
#define BLUEPIN   27

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

//ONE WIRE SETUP

OneWire oneWire(TEMPPIN);
DallasTemperature sensors(&oneWire);

enum systemState{
  IDLE,
  HEATING,
  STABILIZING,
  TARGET_REACHED,
  OVERHEAT
};

systemState currentState = IDLE;
int currentTemp = 0;

void readTemperatureTask(void *pvParameters);
void heaterControlTask(void *pvParameters);
void loggerTask(void *pvParameters);


void setup(){
  Serial.begin(115200);
  BLEDevice::init("ESPHeater");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
    

  
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  
  BLEDevice::startAdvertising();

  sensors.begin();

  pinMode(HEATPIN, OUTPUT);
  pinMode(RESETPIN, INPUT_PULLUP);

  ledcAttach(REDPIN, 5000, 10);  // 5 kHz, 10-bit resolution
  ledcAttach(GREENPIN, 5000, 10);  // 5 kHz, 10-bit resolution
  ledcAttach(BLUEPIN, 5000, 10);  // 5 kHz, 10-bit resolution


  xTaskCreatePinnedToCore(readTemperatureTask, "TempRead", 1000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(heaterControlTask, "HeaterCtrl", 1000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(loggerTask, "Logger", 2000, NULL, 1, NULL, 0);
}
void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
  ledcWrite(REDPIN, r);
  ledcWrite(GREENPIN, g);
  ledcWrite(BLUEPIN, b);
}

void readTemperatureTask(void *pvParameters)
{
  while (1)
  {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void heaterControlTask(void *pvParameters)
{
  while(1){
    if(currentTemp >= OVERHEAT_TEMP)
    {
      currentState = OVERHEAT;
    }
    if(currentState == OVERHEAT && digitalRead(RESETPIN)==LOW)
    {
      currentState = IDLE;
    }

    switch (currentState)
    {
      case IDLE:
        digitalWrite(HEATPIN, LOW);
        if (currentTemp < TARGET_TEMP)
          currentState = HEATING;
        break;

      case HEATING:
        digitalWrite(HEATPIN, HIGH);
        if(currentTemp >= TARGET_TEMP)
          currentState = TARGET_REACHED;
        break;

      case STABILIZING:
        digitalWrite(HEATPIN, LOW);
        if((currentTemp > (TARGET_TEMP - STABILIZING_MARGIN)) && (currentTemp < (TARGET_TEMP - STABILIZING_MARGIN)))
          currentState = STABILIZING;
        else if(currentTemp == TARGET_TEMP)
          currentState = TARGET_REACHED;
        else if(currentTemp <= TARGET_TEMP - STABILIZING_MARGIN)
          currentState = HEATING;
        break;

      case TARGET_REACHED:
        digitalWrite(HEATPIN, LOW);
        if((currentTemp < TARGET_TEMP + 0.9) && (currentTemp > TARGET_TEMP - 0.9 ))
          currentState = TARGET_REACHED;
        else
        currentState = STABILIZING;
        break;

      case OVERHEAT:
        digitalWrite(HEATPIN, LOW);
        break;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void loggerTask(void *pvParameters)
{
  while(1)
  {
    String adstate;  // ← Moved here, before switch

    Serial.printf("Temperature: %d°C State: ", currentTemp);

    switch (currentState)
    {
      case IDLE: 
        Serial.println("IDLE");
        setRGB(0, 0, 255);
        adstate = "IDLE";
        break;

      case HEATING:
        Serial.println("HEATING");
        setRGB(255, 0, 0);
        adstate = "HEATING";
        break;

      case STABILIZING:
        Serial.println("STABILIZING");
        setRGB(255, 255, 0);
        adstate = "STABILIZING";
        break;

      case TARGET_REACHED:
        Serial.println("TARGET REACHED");
        setRGB(0, 255, 0);
        adstate = "TARGET_REACHED";
        break;

      case OVERHEAT:
        Serial.println("OVERHEAT");
        setRGB(255, 255, 255);
        adstate = "OVERHEAT";
        break;
    }

    if (deviceConnected)
    {
      pCharacteristic->setValue(adstate.c_str());  // use c_str() to pass const char*
      pCharacteristic->notify();
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);  // use 500ms instead of 50ms for better BLE behavior
  }
}


void loop()
{
  
}