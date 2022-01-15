#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "e52c58d6-718f-4c36-a913-34571af13739"
#define CHARACTERISTIC_UUID "924dead8-901d-485c-9fb9-0191488641d0"
#define MAX_CHARS 255

//BLE vars
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

//Acc vars
float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

//buffer
char buf[256];

//led
const int ledPin = 10;

void appendSpaces(std::string& s)
{
  int diff = MAX_CHARS - s.size();

  std::string a;
  for(int i = 0; i < diff; ++i  )
  {
    a.push_back(' ');
  }
  s += a;
}

void printStatus(bool isConnected)
{
  M5.Lcd.setCursor(0, 10);
  if (isConnected)
  {
    M5.Lcd.printf("connected  ");
  }
  else
  {
    M5.Lcd.printf("advertising");
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string value = pCharacteristic->getValue();
      Serial.printf("recv size: %d\n", value.size());
      Serial.printf("recv: %s\n", value.c_str());

      if ( value == "LEDON")
      {
        digitalWrite(ledPin, LOW);
      }
      else if ( value == "LEDOFF" )
      {
        digitalWrite(ledPin, HIGH);
      }

      M5.Lcd.setCursor(0, 30);
      appendSpaces(value);//append spaces at the end to refresh the text area
      M5.Lcd.printf("RX: %s", value.c_str());
    }
};

void setup() {
  Serial.begin(115200);

  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("BLE TEST");

  pinMode (ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);//default off

  // Create the BLE Device
  BLEDevice::init("Lab 2 - Group 5");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  M5.Lcd.setCursor(0, 20);

  // notify changed value
  if (deviceConnected)
  {
    sprintf(buf, "%5.2f,%5.2f,%5.2f", accX, accY, accZ);
    pCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    pCharacteristic->notify();
    M5.Lcd.printf("TX: %s", buf);
    printStatus(true);
    delay(500);
  }
  else
  {
    M5.Lcd.printf("                       "); // no msg sent, clear line
    printStatus(false);
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
