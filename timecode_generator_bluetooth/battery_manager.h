#pragma once
#include <Arduino.h>

class BatteryManager {
public:
  BatteryManager();
  void begin();
  int  getPercent();
  int  getMillivolts();
  bool isCharging();
  bool isCritical();    // < 10%
  bool isLow();         // < 20%

private:
  int _readADC();
  int _adcToMv(int adc_val);
  int _mvToPercent(int mv);
};