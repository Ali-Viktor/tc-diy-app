#include "ble_manager.h"
#include "config.h"

BLEManager bleMgr;

// Уникальные ID для наших сервисов (сгенерированы специально для этого проекта)
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_TC_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_BAT_UUID          "16962f9a-c944-46b5-8272-9b2f293b1602"
#define CHAR_FPS_UUID          "828917c1-ea55-4d4a-a66e-fd202cea0645"
#define CHAR_MODE_UUID         "d86241ed-d56b-4e6f-ac34-0f2c41ea41a2"

// Колбэк: Подключение / Отключение телефона
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("[BLE] Client Connected");
    }
    void onDisconnect(BLEServer* pServer) {
      Serial.println("[BLE] Client Disconnected");
      pServer->getAdvertising()->start(); // Снова начинаем раздавать Bluetooth
    }
};

// Колбэк: Телефон прислал новые настройки (сменил FPS или Режим)
class MyCharCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      // Получаем сырые байты данных (т.к. сайт шлет цифры, а не текст)
      uint8_t* pData = pChar->getData();
      size_t len = pChar->getLength();

      if (len > 0) {
        uint8_t val = pData[0];
        
        // Универсальное получение UUID для любой версии ядра
        String uuidStr = pChar->getUUID().toString().c_str(); 

        if (uuidStr.equalsIgnoreCase(CHAR_FPS_UUID)) {
          bleMgr._new_fps = val;
          bleMgr._new_settings = true;
          Serial.printf("[BLE] New FPS received: %d\n", val);
        }
        else if (uuidStr.equalsIgnoreCase(CHAR_MODE_UUID)) {
          bleMgr._new_mode = val;
          bleMgr._new_settings = true;
          Serial.printf("[BLE] New Mode received: %d\n", val);
        }
      }
    }
};

void BLEManager::begin() {
  BLEDevice::init(DEVICE_NAME); // Имя "TC-DIY"
  _pServer = BLEDevice::createServer();
  _pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = _pServer->createService(SERVICE_UUID);

  // Характеристика Таймкода (Телефон только читает)
  _pCharTC = pService->createCharacteristic(
                      CHAR_TC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  _pCharTC->addDescriptor(new BLE2902());

  // Характеристика Батареи (Телефон только читает)
  _pCharBat = pService->createCharacteristic(
                      CHAR_BAT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  _pCharBat->addDescriptor(new BLE2902());

  // Характеристика FPS (Телефон читает и ПИШЕТ)
  _pCharFPS = pService->createCharacteristic(
                      CHAR_FPS_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  _pCharFPS->setCallbacks(new MyCharCallbacks());

  // Характеристика Mode (Телефон читает и ПИШЕТ)
  _pCharMode = pService->createCharacteristic(
                      CHAR_MODE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  _pCharMode->setCallbacks(new MyCharCallbacks());

  pService->start();
  
  // Запускаем рекламу BLE
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Bluetooth Server Started!");
}

void BLEManager::updateTimecode(String tc_str) {
  _pCharTC->setValue(tc_str.c_str());
  _pCharTC->notify();
}

void BLEManager::updateBattery(int pct) {
  uint8_t val = (uint8_t)pct;
  _pCharBat->setValue(&val, 1);
  _pCharBat->notify();
}

bool BLEManager::hasNewSettings() { return _new_settings; }
uint8_t BLEManager::getNewFPS() { return _new_fps; }
uint8_t BLEManager::getNewMode() { return _new_mode; }
void BLEManager::clearSettingsFlag() { _new_settings = false; }