#include "rtc_manager.h"
#include "config.h"

RTCManager::RTCManager() : _initialized(false) {}

bool RTCManager::begin() {
  // Питание RTC через GPIO
  pinMode(PIN_RTC_VCC, OUTPUT);
  digitalWrite(PIN_RTC_VCC, HIGH);
  delay(100);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!_rtc.begin()) {
    Serial.println("[RTC] DS3231 не найден!");
    return false;
  }

  if (_rtc.lostPower()) {
    Serial.println("[RTC] Потеря питания, сброс времени...");
    _rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
  }

  _initialized = true;
  Serial.println("[RTC] DS3231 инициализирован");
  return true;
}

DateTime RTCManager::getDateTime() {
  if (!_initialized) return DateTime(2024, 1, 1, 0, 0, 0);
  return _rtc.now();
}

void RTCManager::setDateTime(DateTime dt) {
  if (_initialized) _rtc.adjust(dt);
}

float RTCManager::getTemperature() {
  if (!_initialized) return 0.0f;
  return _rtc.getTemperature();
}

bool RTCManager::isRunning() { return _initialized; }

void RTCManager::powerCycle() {
  Serial.println("[RTC] Power cycle...");
  digitalWrite(PIN_RTC_VCC, LOW);
  delay(500);
  digitalWrite(PIN_RTC_VCC, HIGH);
  delay(200);
  _rtc.begin();
}