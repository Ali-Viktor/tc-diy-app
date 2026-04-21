#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class BLEManager {
public:
  void begin();
  void updateTimecode(String tc_str);
  void updateBattery(int pct);
  
  // Флаги для связи с главным циклом
  bool    hasNewSettings();
  uint8_t getNewFPS();
  uint8_t getNewMode();
  void    clearSettingsFlag();

private:
  bool _new_settings = false;
  uint8_t _new_fps = 0;
  uint8_t _new_mode = 0;

  BLEServer* _pServer;
  BLECharacteristic* _pCharTC;
  BLECharacteristic* _pCharBat;
  BLECharacteristic* _pCharFPS;
  BLECharacteristic* _pCharMode;

  friend class MyServerCallbacks;
  friend class MyCharCallbacks;
};

extern BLEManager bleMgr;