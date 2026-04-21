#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ltc_engine.h"
#include "config.h"  // Добавили для MAX_PEERS

#define GROUP_COUNT   8
#define PEER_TIMEOUT  5000  // мс — считаем пира мёртвым

struct SyncPacket {
  char     device_name[16];
  uint8_t  device_id;
  uint8_t  group_id;
  uint8_t  mode;
  uint8_t  fps_index;
  uint8_t  hours;
  uint8_t  minutes;
  uint8_t  seconds;
  uint8_t  frames;
  uint64_t timestamp_ms;
  bool     is_master;
};

struct PeerInfo {
  uint8_t  mac[6];
  char     name[16];
  uint64_t last_seen;
  bool     active;
};

class SyncManager {
public:
  SyncManager();
  void    begin(uint8_t group_id, bool is_master);
  void    sendSync(Timecode tc, FrameRate fps);
  void    processSyncPacket(SyncPacket& pkt);
  bool    isSynced();
  int     getPeerCount();
  void    setGroup(uint8_t group_id);
  void    setMaster(bool master);
  void    tick();

  std::function<void(Timecode, FrameRate)> onSyncReceived;

private:
  uint8_t  _group_id;
  bool     _is_master;
  bool     _synced;
  uint64_t _last_sync_time;
  PeerInfo _peers[MAX_PEERS];
  int      _peer_count;

  uint8_t  _broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

  static SyncManager* _instance;
  
  // Обновленные сигнатуры для ESP32 Core 3.x
  static void _onDataSent(const uint8_t* mac, esp_now_send_status_t s);
  static void _onDataRecv(const esp_now_recv_info* info, const uint8_t* data, int len);
};