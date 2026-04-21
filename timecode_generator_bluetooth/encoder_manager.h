#pragma once
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>

enum EncoderEvent {
  ENC_NONE = 0,
  ENC_CW,         // поворот по часовой
  ENC_CCW,        // против часовой
  ENC_CLICK,      // короткое нажатие
  ENC_LONG_PRESS  // долгое нажатие (1.5 сек)
};

class EncoderManager {
public:
  EncoderManager();
  void         begin();
  EncoderEvent getEvent();
  void         resetEncoder();

private:
  AiEsp32RotaryEncoder _encoder;
  unsigned long _btn_press_time;
  bool          _btn_pressed;
  int           _last_value;
};