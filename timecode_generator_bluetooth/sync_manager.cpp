#include "sync_manager.h"
#include "config.h"

SyncManager* SyncManager::_instance = nullptr;

SyncManager::SyncManager()
  : _group_id(0), _is_master(false),
    _synced(false), _last_sync_time(0),
    _peer_count(0) {
  _instance = this;
}

void SyncManager::begin(uint8_t group_id, bool is_master) {
  _group_id  = group_id;
  _is_master = is_master;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("[SYNC] ESP-NOW init failed!");
    return;
  }

  // Принудительное приведение типов для обхода строгости компилятора
  esp_now_register_send_cb((esp_now_send_cb_t)_onDataSent);
  esp_now_register_recv_cb((esp_now_recv_cb_t)_onDataRecv);

  esp_now_peer_info_t peer_info = {};
  memcpy(peer_info.peer_addr, _broadcast_mac, 6);
  peer_info.channel = ESPNOW_CHANNEL;
  peer_info.encrypt = false;
  esp_now_add_peer(&peer_info);

  Serial.printf("[SYNC] ESP-NOW ready. Group:%d Master:%s\n",
    group_id, is_master ? "YES" : "NO");
}

void SyncManager::sendSync(Timecode tc, FrameRate fps) {
  SyncPacket pkt;
  strncpy(pkt.device_name, DEVICE_NAME, 16);
  pkt.device_id    = (uint8_t)(ESP.getEfuseMac() & 0xFF);
  pkt.group_id     = _group_id;
  pkt.fps_index    = (uint8_t)fps;
  pkt.hours        = tc.hours;
  pkt.minutes      = tc.minutes;
  pkt.seconds      = tc.seconds;
  pkt.frames       = tc.frames;
  pkt.timestamp_ms = millis();
  pkt.is_master    = _is_master;

  esp_now_send(_broadcast_mac, (uint8_t*)&pkt, sizeof(SyncPacket));
}

void SyncManager::processSyncPacket(SyncPacket& pkt) {
  if (pkt.group_id != _group_id) return;

  uint8_t my_id = (uint8_t)(ESP.getEfuseMac() & 0xFF);
  if (pkt.device_id == my_id) return;

  bool found = false;
  for (int i = 0; i < _peer_count; i++) {
    if (_peers[i].name[0] != 0 && strcmp(_peers[i].name, pkt.device_name) == 0) {
      _peers[i].last_seen = millis();
      found = true;
      break;
    }
  }
  
  if (!found && _peer_count < MAX_PEERS) {
    strncpy(_peers[_peer_count].name, pkt.device_name, 16);
    _peers[_peer_count].last_seen = millis();
    _peers[_peer_count].active    = true;
    _peer_count++;
  }

  if (!_is_master && pkt.is_master) {
    Timecode tc;
    tc.hours   = pkt.hours;
    tc.minutes = pkt.minutes;
    tc.seconds = pkt.seconds;
    tc.frames  = pkt.frames;
    tc.fps     = (FrameRate)pkt.fps_index;

    _synced          = true;
    _last_sync_time  = millis();

    if (onSyncReceived) {
      onSyncReceived(tc, (FrameRate)pkt.fps_index);
    }
  }
}

void SyncManager::tick() {
  for (int i = 0; i < _peer_count; i++) {
    if (millis() - _peers[i].last_seen > PEER_TIMEOUT) {
      _peers[i].active = false;
    }
  }

  if (_synced && !_is_master) {
    if (millis() - _last_sync_time > PEER_TIMEOUT * 2) {
      _synced = false;
      Serial.println("[SYNC] Sync lost!");
    }
  }
}

bool SyncManager::isSynced()   { return _synced || _is_master; }
int  SyncManager::getPeerCount() {
  int count = 0;
  for (int i = 0; i < _peer_count; i++) {
    if (_peers[i].active) count++;
  }
  return count;
}

void SyncManager::setGroup(uint8_t g) { _group_id = g; }
void SyncManager::setMaster(bool m)   { _is_master = m; }

void SyncManager::_onDataSent(const uint8_t* mac, esp_now_send_status_t s) {}

void SyncManager::_onDataRecv(const esp_now_recv_info* info, const uint8_t* data, int len) {
  if (len == sizeof(SyncPacket) && _instance) {
    SyncPacket pkt;
    memcpy(&pkt, data, sizeof(SyncPacket));
    _instance->processSyncPacket(pkt);
  }
}