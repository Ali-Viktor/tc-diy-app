#include "encoder_manager.h"
#include "config.h"

EncoderManager::EncoderManager()
  : _encoder(PIN_ENC_A, PIN_ENC_B, PIN_ENC_BTN, -1),
    _btn_press_time(0),
    _btn_pressed(false),
    _last_value(0) {}

void IRAM_ATTR readEncoderISR() {
  // Обработчик прерывания для энкодера
}

void EncoderManager::begin() {
  _encoder.begin();
  _encoder.setup(readEncoderISR);
  _encoder.setBoundaries(-1000, 1000, true);
  _encoder.setAcceleration(50);
  Serial.println("[ENC] Encoder initialized");
}

EncoderEvent EncoderManager::getEvent() {
  EncoderEvent event = ENC_NONE;

  // Проверка поворота
  if (_encoder.encoderChanged()) {
    int val = _encoder.readEncoder();
    if (val > _last_value) event = ENC_CW;
    if (val < _last_value) event = ENC_CCW;
    _last_value = val;
  }

  // Проверка кнопки
  if (_encoder.isEncoderButtonDown()) {
    if (!_btn_pressed) {
      _btn_pressed    = true;
      _btn_press_time = millis();
    }
  } else {
    if (_btn_pressed) {
      _btn_pressed = false;
      unsigned long held = millis() - _btn_press_time;
      if (held >= 1500) {
        event = ENC_LONG_PRESS;
      } else {
        event = ENC_CLICK;
      }
    }
  }

  return event;
}

void EncoderManager::resetEncoder() {
  _encoder.reset();
  _last_value = 0;
}