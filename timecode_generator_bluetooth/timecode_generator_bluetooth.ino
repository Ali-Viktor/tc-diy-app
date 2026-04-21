// ============================================================
//  DIY SMPTE LTC Timecode Generator
//  Аналог Saramonic TC-NEO
//  Платформа : ESP32-S3 | FW Version: 1.0.0
// ============================================================

#include "config.h"
#include "ltc_engine.h"
#include "rtc_manager.h"
#include "display_manager.h"
#include "encoder_manager.h"
#include "sync_manager.h"
#include "mic_manager.h"
#include "battery_manager.h"
#include <Preferences.h>
#include "ble_manager.h"
// ============================================================
//  Объекты модулей
// ============================================================
LTCEngine      ltc;
RTCManager     rtc;
DisplayManager display;
EncoderManager encoder;
SyncManager    syncMgr; // ПЕРЕИМЕНОВАЛИ С sync НА syncMgr
MicManager     mic;
BatteryManager battery;
Preferences    prefs;

// ============================================================
//  Состояние системы
// ============================================================
struct AppState {
  FrameRate fps        = FPS_25;
  uint8_t   mode       = 0;   // 0=MasterRun, 1=AutoJam, 2=JamOnceLock
  uint8_t   group_id   = 0;
  bool      is_master  = true;
  bool      running    = false;

  bool      jam_active = false;
  Timecode  jam_tc;

  int       menu_item  = 0;
  int       fps_select = FPS_25;
  int       mode_select = 0;
  int       group_select = 0;
} state;

// ============================================================
//  Таймеры обновления
// ============================================================
unsigned long last_display_update  = 0;
unsigned long last_sync_send       = 0;
unsigned long last_jam_check       = 0;

const int DISPLAY_UPDATE_MS = 100;
const int SYNC_SEND_MS      = 1000;

// ============================================================
//  Прототипы функций
// ============================================================
void loadSettings();
void saveSettings();
void handleEncoder();
void handleMenu(EncoderEvent ev);
void handleFPSMenu(EncoderEvent ev);
void handleModeMenu(EncoderEvent ev);
void handleGroupMenu(EncoderEvent ev);
void jamSync();
void updateDisplay();

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[BOOT] DIY TC Generator v" FW_VERSION);

  loadSettings();
  battery.begin();
  Wire.begin(PIN_SDA, PIN_SCL);

  if (!rtc.begin()) Serial.println("[BOOT] RTC failed, using default time");
  if (!display.begin()) Serial.println("[BOOT] Display failed!");

  encoder.begin();
  mic.begin();

  syncMgr.begin(state.group_id, state.is_master);
  syncMgr.onSyncReceived = [](Timecode tc, FrameRate fps) {
    if (state.mode == 1 || (state.mode == 2 && !state.jam_active)) {
      ltc.setTimecode(tc);
      ltc.setFrameRate(fps);
      state.fps       = fps;
      state.jam_active = true;
      Serial.println("[JAM] Timecode synced from master");
    }
  };

  DateTime now = rtc.getDateTime();
  Timecode start_tc = {now.hour(), now.minute(), now.second(), 0, state.fps, 
                      (state.fps == FPS_29_97_DF || state.fps == FPS_59_94_DF)};

  ltc.begin(PIN_LTC_OUT, state.fps);
  ltc.setTimecode(start_tc);

  if (state.mode == 0) {
    ltc.start();
    state.running = true;
    Serial.println("[LTC] Started in Master Run mode");
  }

  display.showMessage("READY", "LTC Generator Active", 1000);
   // Запуск Bluetooth
  bleMgr.begin();
  // Отправляем начальные значения
  uint8_t initial_fps = (uint8_t)state.fps;
  uint8_t initial_mode = state.mode;
  // (Внутренние функции BLE сохранят их, чтобы телефон при подключении увидел текущие)
   Serial.println("[BOOT] Setup complete!");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  unsigned long now_ms = millis();
  handleEncoder();

  if (now_ms - last_display_update >= DISPLAY_UPDATE_MS) {
    last_display_update = now_ms;
    updateDisplay();
  }

  if (state.is_master && state.running) {
    if (now_ms - last_sync_send >= SYNC_SEND_MS) {
      last_sync_send = now_ms;
      syncMgr.sendSync(ltc.getTimecode(), state.fps);
    }
  }

  syncMgr.tick();

  if (state.mode == 1) {
    if (now_ms - last_jam_check >= 500) {
      last_jam_check = now_ms;
      jamSync();
    }
  }

  if (battery.isCritical()) {
    display.showMessage("!! LOW BATTERY !!", "Connect USB-C charge", 0);
    delay(3000);
  }
    // 1. Отправляем таймкод и батарею по Bluetooth (каждые 200 мс)
  static unsigned long last_ble_update = 0;
  if (now_ms - last_ble_update >= 200) {
    last_ble_update = now_ms;
    bleMgr.updateTimecode(ltc.timecodeToString(ltc.getTimecode()));
    bleMgr.updateBattery(battery.getPercent());
  }

  // 2. Проверяем, не прислал ли телефон новые настройки
  if (bleMgr.hasNewSettings()) {
    state.fps = (FrameRate)bleMgr.getNewFPS();
    state.mode = bleMgr.getNewMode();
    
    // Применяем новый FPS
    ltc.setFrameRate(state.fps);
    
    // Применяем новый режим
    if (state.mode == 0) {
      state.is_master = true; syncMgr.setMaster(true);
      if (!ltc.isRunning()) { ltc.start(); state.running = true; }
    } else {
      state.is_master = false; syncMgr.setMaster(false);
    }
    
    saveSettings();
    bleMgr.clearSettingsFlag();
    display.showMessage("BLE SYNC", "Settings Updated", 1500);
    display.setScreen(SCREEN_MAIN);
  }
}

// ============================================================
//  ОБНОВЛЕНИЕ ДИСПЛЕЯ
// ============================================================
void updateDisplay() {
  switch (display.getScreen()) {
    case SCREEN_MAIN:
      display.showMain(
        ltc.getTimecode(), ltc.getFPSString(state.fps),
        state.mode == 0 ? "MSTR" : state.mode == 1 ? "AJAM" : "JLCK",
        battery.getPercent(), syncMgr.isSynced(), state.group_id
      );
      break;
    case SCREEN_MENU:
      display.showMenu(state.menu_item);
      break;
    case SCREEN_FPS_SELECT:
      display.showFPSSelect(state.fps_select);
      break;
    case SCREEN_MODE_SELECT:
      display.showModeSelect(state.mode_select);
      break;
    case SCREEN_SYNC_STATUS:
      display.showSyncStatus(syncMgr.getPeerCount(), state.is_master);
      break;
    default:
      break;
  }
}

// ============================================================
//  ОБРАБОТКА ЭНКОДЕРА
// ============================================================
void handleEncoder() {
  EncoderEvent ev = encoder.getEvent();
  if (ev == ENC_NONE) return;

  switch (display.getScreen()) {
    case SCREEN_MAIN:
      if (ev == ENC_CLICK) {
        display.setScreen(SCREEN_MENU);
        state.menu_item = 0;
      } else if (ev == ENC_LONG_PRESS) {
        if (state.running) {
          ltc.stop();
          state.running = false;
          display.showMessage("STOPPED", "LTC Output off", 1000);
        } else {
          ltc.start();
          state.running = true;
          display.showMessage("RUNNING", "LTC Output on", 1000);
        }
      }
      break;
    case SCREEN_MENU:
      handleMenu(ev);
      break;
    case SCREEN_FPS_SELECT:
      handleFPSMenu(ev);
      break;
    case SCREEN_MODE_SELECT:
      handleModeMenu(ev);
      break;
    case SCREEN_SYNC_STATUS:
      if (ev == ENC_CLICK) {
        display.setScreen(SCREEN_MAIN);
      } else if (ev == ENC_CW || ev == ENC_CCW) {
        state.is_master = !state.is_master;
        syncMgr.setMaster(state.is_master);
        saveSettings();
      }
      break;
    default:
      if (ev == ENC_CLICK) display.setScreen(SCREEN_MAIN);
      break;
  }
}

// ============================================================
//  МЕНЮ
// ============================================================
void handleMenu(EncoderEvent ev) {
  if (ev == ENC_CW)  state.menu_item++;
  if (ev == ENC_CCW) state.menu_item--;
  state.menu_item = constrain(state.menu_item, 0, 6);

  if (ev == ENC_CLICK) {
    switch (state.menu_item) {
      case 0:
        state.fps_select = (int)state.fps;
        display.setScreen(SCREEN_FPS_SELECT);
        break;
      case 1:
        state.mode_select = state.mode;
        display.setScreen(SCREEN_MODE_SELECT);
        break;
      case 2:
        display.setScreen(SCREEN_SYNC_STATUS);
        break;
      case 3:
        {
          DateTime now = rtc.getDateTime();
          Timecode tc = {now.hour(), now.minute(), now.second(), 0, state.fps, false};
          ltc.setTimecode(tc);
          display.showMessage("SET TIME", "Synced to RTC clock", 2000);
          display.setScreen(SCREEN_MAIN);
        }
        break;
      case 4:
        state.group_select = state.group_id;
        display.setScreen(SCREEN_SETTINGS);
        break;
      case 5:
        display.showMessage("SETTINGS", "Coming soon...", 1500);
        display.setScreen(SCREEN_MAIN);
        break;
      case 6:
        display.showMessage("TC-DIY v" FW_VERSION, "SMPTE LTC Generator\nESP32-S3 + DS3231", 3000);
        display.setScreen(SCREEN_MAIN);
        break;
    }
  }
  if (ev == ENC_LONG_PRESS) display.setScreen(SCREEN_MAIN);
}

// ============================================================
//  МЕНЮ ВЫБОРА FPS
// ============================================================
void handleFPSMenu(EncoderEvent ev) {
  if (ev == ENC_CW)  state.fps_select++;
  if (ev == ENC_CCW) state.fps_select--;
  state.fps_select = constrain(state.fps_select, 0, FPS_COUNT - 1);

  if (ev == ENC_CLICK) {
    state.fps = (FrameRate)state.fps_select;
    ltc.setFrameRate(state.fps);
    saveSettings();
    display.showMessage("FPS SET", ltc.getFPSString(state.fps) + " fps", 1500);
    display.setScreen(SCREEN_MAIN);
  }
  if (ev == ENC_LONG_PRESS) display.setScreen(SCREEN_MENU);
}

// ============================================================
//  МЕНЮ ВЫБОРА РЕЖИМА
// ============================================================
void handleModeMenu(EncoderEvent ev) {
  if (ev == ENC_CW)  state.mode_select++;
  if (ev == ENC_CCW) state.mode_select--;
  state.mode_select = constrain(state.mode_select, 0, 2);

  if (ev == ENC_CLICK) {
    state.mode = state.mode_select;
    state.jam_active = false;

    switch (state.mode) {
      case 0:
        state.is_master = true;
        syncMgr.setMaster(true);
        if (!ltc.isRunning()) {
          ltc.start();
          state.running = true;
        }
        break;
      case 1:
        state.is_master = false;
        syncMgr.setMaster(false);
        break;
      case 2:
        state.is_master = false;
        syncMgr.setMaster(false);
        break;
    }

    saveSettings();
    const char* mode_names[] = {"Master Run", "Auto Jam", "Jam Once Lock"};
    display.showMessage("MODE SET", mode_names[state.mode], 1500);
    display.setScreen(SCREEN_MAIN);
  }
  if (ev == ENC_LONG_PRESS) display.setScreen(SCREEN_MENU);
}

// ============================================================
//  JAM SYNC (Заглушка для аналогового входа)
// ============================================================
void jamSync() {
  if (analogRead(PIN_LTC_IN) > 500) {
    Serial.println("[JAM] LTC input signal detected");
  }
}

// ============================================================
//  СОХРАНЕНИЕ / ЗАГРУЗКА
// ============================================================
void loadSettings() {
  prefs.begin("tc_settings", true);
  state.fps      = (FrameRate)prefs.getInt("fps",  FPS_25);
  state.mode     = prefs.getInt("mode", 0);
  state.group_id = prefs.getInt("group", 0);
  state.is_master = prefs.getBool("master", true);
  prefs.end();
}

void saveSettings() {
  prefs.begin("tc_settings", false);
  prefs.putInt("fps",   (int)state.fps);
  prefs.putInt("mode",  state.mode);
  prefs.putInt("group", state.group_id);
  prefs.putBool("master", state.is_master);
  prefs.end();
}