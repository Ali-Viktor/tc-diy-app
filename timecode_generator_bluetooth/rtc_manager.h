#pragma once
#include <Arduino.h>
#include <RTClib.h>

class RTCManager {
public:
  RTCManager();
  bool     begin();
  DateTime getDateTime();
  void     setDateTime(DateTime dt);
  float    getTemperature();
  bool     isRunning();
  void     powerCycle();   // перезапуск через GPIO 32

private:
  RTC_DS3231 _rtc;
  bool       _initialized;
};