#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ltc_engine.h"

enum DisplayScreen {
  SCREEN_MAIN = 0,
  SCREEN_MENU,
  SCREEN_FPS_SELECT,
  SCREEN_MODE_SELECT,
  SCREEN_SYNC_STATUS,
  SCREEN_SETTINGS,
  SCREEN_ABOUT
};

class DisplayManager {
public:
  DisplayManager();
  bool   begin();
  void   showMain(Timecode tc, String fps_str,
                  String mode_str, int battery_pct,
                  bool synced, int group_id);
  void   showMenu(int selected_item);
  void   showFPSSelect(int selected_fps);
  void   showModeSelect(int selected_mode);
  void   showSyncStatus(int peers, bool is_master);
  void   showSplash();
  void   showMessage(String title, String msg, int delay_ms = 0);
  void   setScreen(DisplayScreen screen);
  DisplayScreen getScreen();

private:
  Adafruit_SSD1306 _display;
  DisplayScreen    _current_screen;

  void _drawBatteryIcon(int pct, int x, int y);
  void _drawSyncIcon(bool synced, int x, int y);
};