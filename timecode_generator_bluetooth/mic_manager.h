#pragma once
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_SAMPLE_RATE   44100
#define I2S_BUFFER_SIZE   64
#define I2S_PORT          I2S_NUM_0

class MicManager {
public:
  MicManager();
  bool    begin();
  int32_t readSample();
  void    readBuffer(int32_t* buf, size_t len);
  bool    isActive();

private:
  bool _initialized;
};