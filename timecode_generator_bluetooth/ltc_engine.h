#pragma once
#include <Arduino.h>

enum FrameRate {
  FPS_23_98 = 0, FPS_24, FPS_25,
  FPS_29_97, FPS_29_97_DF, FPS_30,
  FPS_47_95, FPS_48, FPS_50,
  FPS_59_94, FPS_59_94_DF, FPS_60,
  FPS_COUNT
};

struct Timecode {
  uint8_t hours; uint8_t minutes; uint8_t seconds; uint8_t frames;
  FrameRate fps; bool drop_frame;
};

struct LTCFrame {
  uint8_t frames_units:4; uint8_t user1:4;
  uint8_t frames_tens:2;  uint8_t drop_frame:1; uint8_t color_frame:1; uint8_t user2:4;
  uint8_t secs_units:4;   uint8_t user3:4;
  uint8_t secs_tens:3;    uint8_t biphase_mark:1; uint8_t user4:4;
  uint8_t mins_units:4;   uint8_t user5:4;
  uint8_t mins_tens:3;    uint8_t binary_group:1; uint8_t user6:4;
  uint8_t hours_units:4;  uint8_t user7:4;
  uint8_t hours_tens:2;   uint8_t reserved:1; uint8_t clock_flag:1; uint8_t user8:4;
  uint16_t sync_word;
};

class LTCEngine {
public:
  LTCEngine();
  void     begin(uint8_t output_pin, FrameRate fps);
  void     setTimecode(Timecode tc);
  void     setFrameRate(FrameRate fps);
  void     tick();
  void     start();
  void     stop();

  Timecode getTimecode();
  float    getFPSValue(FrameRate fps);
  String   getFPSString(FrameRate fps);
  String   timecodeToString(Timecode tc);
  bool     isRunning();

private:
  uint8_t   _pin;
  FrameRate _fps;
  Timecode  _tc;
  bool      _running;

  uint8_t   _bits[80];
  int       _bit_index;
  bool      _last_level;
  
  // Предварительно вычисленные целые значения для избежания float-математики в прерываниях
  uint8_t   _fps_int;
  uint8_t   _fps_skip;

  void  _updateFPSVars();
  void  _encodeLTCFrame();
  void  _packBits(LTCFrame& frame);
  void  _advanceFrame();
  void  _handleDropFrame();

  const float FPS_VALUES[FPS_COUNT] = {
    23.976f, 24.0f, 25.0f, 29.97f, 29.97f, 30.0f,
    47.95f, 48.0f, 50.0f, 59.94f, 59.94f, 60.0f
  };
};