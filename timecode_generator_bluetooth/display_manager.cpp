#include "display_manager.h"
#include "config.h"

const char* MENU_ITEMS[] = {
  "FPS",
  "Mode",
  "Sync",
  "Set Time",
  "Group",
  "Settings",
  "About"
};
const int MENU_COUNT = 7;

DisplayManager::DisplayManager()
  : _display(SCREEN_WIDTH, SCREEN_HEIGHT,
             &Wire, OLED_RESET),
    _current_screen(SCREEN_MAIN) {}

bool DisplayManager::begin() {
  if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[DISPLAY] SSD1306 не найден!");
    return false;
  }
  _display.setTextColor(SSD1306_WHITE);
  showSplash();
  return true;
}

// ============================================================
//  Главный экран
// ============================================================
void DisplayManager::showMain(Timecode tc, String fps_str,
    String mode_str, int battery_pct,
    bool synced, int group_id) {

  _display.clearDisplay();

  // === Строка 1: Таймкод (Часы:Минуты:Секунды) — КРУПНО ===
  _display.setTextSize(2);
  _display.setCursor(0, 0);
  char tc_main[10];
  snprintf(tc_main, sizeof(tc_main), "%02d:%02d:%02d", tc.hours, tc.minutes, tc.seconds);
  _display.print(tc_main);

  // === Кадры (Фреймы) — МЕЛКО, выровнено по низу ===
  _display.setTextSize(1);
  _display.setCursor(96, 7); // Сдвигаем вправо после секунд
  char sep = tc.drop_frame ? ';' : ':';
  _display.printf("%c%02d", sep, tc.frames);

  // === Строка 2: FPS + режим ===
  _display.setTextSize(1);
  _display.setCursor(0, 20);
  _display.print(fps_str);
  _display.print("fps ");
  _display.print(mode_str);

  // === Строка 3: Батарея + синхронизация ===
  _drawBatteryIcon(battery_pct, 100, 20);
  _display.setCursor(0, 30);
  _display.printf("BAT:%d%%", battery_pct);

  // === Строка 4: Статус синхронизации + группа ===
  _display.setCursor(0, 42);
  if (synced) {
    _display.print("SYNC:LOCK");
  } else {
    _display.print("SYNC:FREE");
  }
  _display.printf("  GRP:%d", group_id);

  // === Разделитель ===
  _display.drawLine(0, 39, 127, 39, SSD1306_WHITE);

  // === Иконка синхронизации ===
  _drawSyncIcon(synced, 112, 42);

  // === Нижняя строка: подсказка ===
  _display.setCursor(0, 56);
  _display.print("[BTN] Menu");

  _display.display();
}

// ============================================================
//  Экран меню
// ============================================================
void DisplayManager::showMenu(int selected_item) {
  _display.clearDisplay();
  _display.setTextSize(1);

  _display.setCursor(0, 0);
  _display.print("=== MENU ===");
  _display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  int start = max(0, selected_item - 2);
  int end   = min(MENU_COUNT - 1, start + 4);

  for (int i = start; i <= end; i++) {
    int y = 12 + (i - start) * 10;
    if (i == selected_item) {
      _display.fillRect(0, y - 1, 127, 10, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }
    _display.setCursor(4, y);
    _display.print(MENU_ITEMS[i]);
  }

  _display.setTextColor(SSD1306_WHITE);
  _display.display();
}

// ============================================================
//  Выбор FPS
// ============================================================
void DisplayManager::showFPSSelect(int selected_fps) {
  const char* fps_names[] = {
    "23.98", "24", "25", "29.97",
    "29.97DF", "30", "47.95", "48",
    "50", "59.94", "59.94DF", "60"
  };

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setCursor(0, 0);
  _display.print("=== FPS ===");
  _display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  int start = max(0, selected_fps - 2);
  int end   = min(FPS_COUNT - 1, start + 4);

  for (int i = start; i <= end; i++) {
    int y = 12 + (i - start) * 10;
    if (i == selected_fps) {
      _display.fillRect(0, y - 1, 127, 10, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }
    _display.setCursor(4, y);
    _display.print(fps_names[i]);
  }

  _display.setTextColor(SSD1306_WHITE);
  _display.display();
}

// ============================================================
//  Выбор режима
// ============================================================
void DisplayManager::showModeSelect(int selected_mode) {
  const char* modes[] = {
    "Master Run",
    "Auto Jam",
    "Jam Once & Lock"
  };

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setCursor(0, 0);
  _display.print("=== MODE ===");
  _display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  for (int i = 0; i < 3; i++) {
    int y = 14 + i * 14;
    if (i == selected_mode) {
      _display.fillRect(0, y - 1, 127, 12, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }
    _display.setCursor(4, y);
    _display.print(modes[i]);
  }

  _display.setTextColor(SSD1306_WHITE);
  _display.display();
}

// ============================================================
//  Статус синхронизации
// ============================================================
void DisplayManager::showSyncStatus(int peers, bool is_master) {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setCursor(0, 0);
  _display.print("=== SYNC ===");
  _display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  _display.setCursor(0, 14);
  _display.printf("Role: %s", is_master ? "MASTER" : "SLAVE");
  _display.setCursor(0, 26);
  _display.printf("Peers: %d", peers);
  _display.setCursor(0, 38);
  _display.print("Protocol: ESP-NOW");
  _display.setCursor(0, 50);
  _display.print("BT: Active");
  _display.display();
}

// ============================================================
//  Заставка при запуске
// ============================================================
void DisplayManager::showSplash() {
  _display.clearDisplay();
  _display.setTextSize(2);
  _display.setCursor(10, 10);
  _display.print("TC-DIY");
  _display.setTextSize(1);
  _display.setCursor(20, 32);
  _display.print("SMPTE LTC Generator");
  _display.setCursor(30, 44);
  _display.printf("FW v%s", FW_VERSION);
  _display.setCursor(10, 56);
  _display.print("Initializing...");
  _display.display();
  delay(2000);
}

// ============================================================
//  Сообщение
// ============================================================
void DisplayManager::showMessage(String title, String msg,
                                  int delay_ms) {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setCursor(0, 8);
  _display.print(title);
  _display.drawLine(0, 18, 127, 18, SSD1306_WHITE);
  _display.setCursor(0, 24);
  _display.print(msg);
  _display.display();
  if (delay_ms > 0) delay(delay_ms);
}

void DisplayManager::setScreen(DisplayScreen s) {
  _current_screen = s;
}

DisplayScreen DisplayManager::getScreen() {
  return _current_screen;
}

// ============================================================
//  Иконка батареи
// ============================================================
void DisplayManager::_drawBatteryIcon(int pct, int x, int y) {
  _display.drawRect(x, y, 20, 10, SSD1306_WHITE);
  _display.fillRect(x + 20, y + 3, 2, 4, SSD1306_WHITE);
  int fill = map(pct, 0, 100, 0, 18);
  _display.fillRect(x + 1, y + 1, fill, 8, SSD1306_WHITE);
}

// ============================================================
//  Иконка синхронизации
// ============================================================
void DisplayManager::_drawSyncIcon(bool synced, int x, int y) {
  if (synced) {
    // Рисуем закрытый замок
    _display.drawRect(x, y + 4, 10, 8, SSD1306_WHITE);
    // Рисуем дужку замка линиями
    _display.drawLine(x + 2, y + 4, x + 2, y + 2, SSD1306_WHITE);
    _display.drawLine(x + 7, y + 4, x + 7, y + 2, SSD1306_WHITE);
    _display.drawLine(x + 3, y + 1, x + 6, y + 1, SSD1306_WHITE);
  } else {
    // Замок открыт — знак вопроса
    _display.setCursor(x, y);
    _display.print("?");
  }
}