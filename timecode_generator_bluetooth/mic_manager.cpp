#include "mic_manager.h"
#include "config.h"

MicManager::MicManager() : _initialized(false) {}

bool MicManager::begin() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = I2S_BUFFER_SIZE,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = PIN_I2S_SCK,
    .ws_io_num    = PIN_I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_I2S_DATA
  };

  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config,
                                      0, NULL);
  if (err != ESP_OK) {
    Serial.printf("[MIC] I2S install failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[MIC] I2S pin config failed: %d\n", err);
    return false;
  }

  _initialized = true;
  Serial.println("[MIC] INMP441 I2S initialized");
  return true;
}

int32_t MicManager::readSample() {
  if (!_initialized) return 0;
  int32_t sample = 0;
  size_t  bytes_read;
  i2s_read(I2S_PORT, &sample, sizeof(sample),
           &bytes_read, 10 / portTICK_PERIOD_MS);
  return sample >> 8;  // INMP441 выдаёт данные в старших битах
}

void MicManager::readBuffer(int32_t* buf, size_t len) {
  if (!_initialized) {
    memset(buf, 0, len * sizeof(int32_t));
    return;
  }
  size_t bytes_read;
  i2s_read(I2S_PORT, buf, len * sizeof(int32_t),
           &bytes_read, 10 / portTICK_PERIOD_MS);
  // Сдвиг данных INMP441
  for (size_t i = 0; i < len; i++) {
    buf[i] >>= 8;
  }
}

bool MicManager::isActive() { return _initialized; }