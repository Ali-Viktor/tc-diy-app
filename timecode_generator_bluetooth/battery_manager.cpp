#include "battery_manager.h"
#include "config.h"

BatteryManager::BatteryManager() {}

void BatteryManager::begin() {
  pinMode(PIN_BATTERY_ADC, INPUT);
  analogSetAttenuation(ADC_11db);  // 0-3.9V диапазон
  Serial.println("[BAT] Battery monitor ready");
}

int BatteryManager::_readADC() {
  // Усредняем 10 замеров
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(PIN_BATTERY_ADC);
    delay(2);
  }
  return (int)(sum / 10);
}

int BatteryManager::_adcToMv(int adc_val) {
  // ESP32 ADC: 12 бит, 0-4095 → 0-3900 мВ (с 11dB)
  float voltage = (adc_val / 4095.0f) * 3900.0f;
  // Учитываем делитель напряжения R1/R2
  float ratio = (float)(BATTERY_R1 + BATTERY_R2) /
                (float)BATTERY_R2;
  return (int)(voltage * ratio);
}

int BatteryManager::_mvToPercent(int mv) {
  mv = constrain(mv, BATTERY_MIN_MV, BATTERY_MAX_MV);
  return map(mv, BATTERY_MIN_MV, BATTERY_MAX_MV, 0, 100);
}

int BatteryManager::getMillivolts() {
  return _adcToMv(_readADC());
}

int BatteryManager::getPercent() {
  return _mvToPercent(getMillivolts());
}

bool BatteryManager::isCharging() {
  // Определяем зарядку по напряжению > 4.1В или
  // через отдельный пин зарядного модуля (если есть)
  return getMillivolts() > 4100;
}

bool BatteryManager::isCritical() { return getPercent() < 10; }
bool BatteryManager::isLow()      { return getPercent() < 20; }