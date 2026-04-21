#include "ltc_engine.h"
#include "config.h"
#include "driver/gpio.h" // Добавили для быстрого управления пинами

LTCEngine* ltc_instance = nullptr;
hw_timer_t* ltc_timer = nullptr;

void IRAM_ATTR onLTCTimer() {
  if (ltc_instance) ltc_instance->tick();
}

LTCEngine::LTCEngine() {
  _running    = false;
  _bit_index  = 0;
  _last_level = false;
  _fps_int    = 25;
  _fps_skip   = 0;
  ltc_instance = this;
  _tc = {0, 0, 0, 0, FPS_25, false};
}

void LTCEngine::begin(uint8_t output_pin, FrameRate fps) {
  _pin = output_pin;
  _fps = fps;
  _updateFPSVars();
  
  // Настраиваем пин через GPIO драйвер
  gpio_reset_pin((gpio_num_t)_pin);
  gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)_pin, 0);
  
  _encodeLTCFrame();
}

void LTCEngine::start() {
  _running = true;
  float fps_val   = getFPSValue(_fps);
  float bit_rate  = fps_val * 80.0f;
  uint32_t half_bit_us = (uint32_t)(500000.0f / bit_rate);

  ltc_timer = timerBegin(1000000); 
  timerAttachInterrupt(ltc_timer, &onLTCTimer);
  timerAlarm(ltc_timer, half_bit_us, true, 0);
}

void LTCEngine::stop() {
  _running = false;
  if (ltc_timer) {
    timerEnd(ltc_timer);
    ltc_timer = nullptr;
  }
  gpio_set_level((gpio_num_t)_pin, 0);
}

void IRAM_ATTR LTCEngine::tick() {
  if (!_running) return;

  static bool half_bit = false;

  if (!half_bit) {
    if (_bits[_bit_index] == 1) _last_level = !_last_level;
    gpio_set_level((gpio_num_t)_pin, _last_level ? 1 : 0);
    half_bit = true;
  } else {
    if (_bits[_bit_index] == 1) {
      _last_level = !_last_level;
    } else {
      _last_level = !_last_level;
    }
    gpio_set_level((gpio_num_t)_pin, _last_level ? 1 : 0);
    half_bit = false;

    _bit_index++;
    if (_bit_index >= 80) {
      _bit_index = 0;
      _advanceFrame();
      _encodeLTCFrame();
    }
  }
}

// Теперь заполняем байты напрямую! Никаких структур и memset.
void IRAM_ATTR LTCEngine::_encodeLTCFrame() {
  uint8_t b[10];
  // Заполняем массив нулями вручную (компилятор не подставит FPU)
  for(int i = 0; i < 10; i++) b[i] = 0;

  b[0] = (_tc.frames % 10) & 0x0F;
  b[1] = (_tc.frames / 10) & 0x03;
  if (_tc.drop_frame) b[1] |= 0x04;
  
  b[2] = (_tc.seconds % 10) & 0x0F;
  b[3] = (_tc.seconds / 10) & 0x07;
  
  b[4] = (_tc.minutes % 10) & 0x0F;
  b[5] = (_tc.minutes / 10) & 0x07;
  
  b[6] = (_tc.hours % 10) & 0x0F;
  b[7] = (_tc.hours / 10) & 0x03;
  
  b[8] = 0xFD; // Синхро-слово 0x3FFD (нижний байт)
  b[9] = 0x3F; // Синхро-слово (верхний байт)

  for (int i = 0; i < 80; i++) {
    _bits[i] = (b[i / 8] >> (i % 8)) & 0x01;
  }
}

void IRAM_ATTR LTCEngine::_packBits(LTCFrame& frame) {
  // Функция больше не используется, оставлена пустой для совместимости
}

void IRAM_ATTR LTCEngine::_advanceFrame() {
  if (_tc.drop_frame) {
    _handleDropFrame();
    return;
  }

  _tc.frames++;
  if (_tc.frames >= _fps_int) {
    _tc.frames = 0;
    _tc.seconds++;
    if (_tc.seconds >= 60) {
      _tc.seconds = 0;
      _tc.minutes++;
      if (_tc.minutes >= 60) {
        _tc.minutes = 0;
        _tc.hours++;
        if (_tc.hours >= 24) _tc.hours = 0;
      }
    }
  }
}

void IRAM_ATTR LTCEngine::_handleDropFrame() {
  _tc.frames++;
  if (_tc.frames >= _fps_int) {
    _tc.frames = 0;
    _tc.seconds++;
    if (_tc.seconds >= 60) {
      _tc.seconds = 0;
      _tc.minutes++;

      if (_tc.minutes % 10 != 0) {
        _tc.frames = _fps_skip;
      }

      if (_tc.minutes >= 60) {
        _tc.minutes = 0;
        _tc.hours++;
        if (_tc.hours >= 24) _tc.hours = 0;
      }
    }
  }
}

void LTCEngine::_updateFPSVars() {
  switch (_fps) {
    case FPS_23_98:    _fps_int = 24; _fps_skip = 0; break;
    case FPS_24:       _fps_int = 24; _fps_skip = 0; break;
    case FPS_25:       _fps_int = 25; _fps_skip = 0; break;
    case FPS_29_97:    _fps_int = 30; _fps_skip = 0; break;
    case FPS_29_97_DF: _fps_int = 30; _fps_skip = 2; break;
    case FPS_30:       _fps_int = 30; _fps_skip = 0; break;
    case FPS_47_95:    _fps_int = 48; _fps_skip = 0; break;
    case FPS_48:       _fps_int = 48; _fps_skip = 0; break;
    case FPS_50:       _fps_int = 50; _fps_skip = 0; break;
    case FPS_59_94:    _fps_int = 60; _fps_skip = 0; break;
    case FPS_59_94_DF: _fps_int = 60; _fps_skip = 4; break;
    case FPS_60:       _fps_int = 60; _fps_skip = 0; break;
    default:           _fps_int = 25; _fps_skip = 0; break;
  }
}

float LTCEngine::getFPSValue(FrameRate fps) { return FPS_VALUES[fps]; }

String LTCEngine::getFPSString(FrameRate fps) {
  switch (fps) {
    case FPS_23_98: return "23.98"; case FPS_24: return "24"; case FPS_25: return "25";
    case FPS_29_97: return "29.97"; case FPS_29_97_DF: return "29.97DF"; case FPS_30: return "30";
    case FPS_47_95: return "47.95"; case FPS_48: return "48"; case FPS_50: return "50";
    case FPS_59_94: return "59.94"; case FPS_59_94_DF: return "59.94DF"; case FPS_60: return "60";
    default: return "25";
  }
}

String LTCEngine::timecodeToString(Timecode tc) {
  char buf[16];
  char sep = tc.drop_frame ? ';' : ':';
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d%c%02d", tc.hours, tc.minutes, tc.seconds, sep, tc.frames);
  return String(buf);
}

void LTCEngine::setTimecode(Timecode tc) {
  _tc = tc;
  _encodeLTCFrame();
}

void LTCEngine::setFrameRate(FrameRate fps) {
  bool was_running = _running;
  if (was_running) stop();
  _fps = fps;
  _updateFPSVars();
  _tc.drop_frame = (fps == FPS_29_97_DF || fps == FPS_59_94_DF);
  _encodeLTCFrame();
  if (was_running) start();
}

Timecode LTCEngine::getTimecode() { return _tc; }
bool     LTCEngine::isRunning()   { return _running; }