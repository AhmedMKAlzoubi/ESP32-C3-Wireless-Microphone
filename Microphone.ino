#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ----------- MIC (INMP441) PINS -----------
#define I2S_WS    3    // LRCL / WS
#define I2S_SCK   2    // BCLK / SCK
#define I2S_SD    4    // DOUT from INMP441

#define I2S_PORT       I2S_NUM_0
#define SAMPLE_RATE    16000
#define SAMPLES_READ   160    // 10 ms of audio at 16 kHz

// ----------- OLED PINS -----------
#define OLED_SDA   8
#define OLED_SCL   9
#define OLED_ADDR  0x3C

SSD1306Wire display(OLED_ADDR, OLED_SDA, OLED_SCL);

// ----------- WIFI + UDP -----------
const char* ssid       = "Ahmed";
const char* password   = "0508981447ahmed";
const char* udpAddress = "192.168.1.44";   // <-- your laptop IP
const int   udpPort    = 5005;

WiFiUDP udp;

// ----------- I2S SETUP -----------
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 4,
    .dma_buf_len          = SAMPLES_READ,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
}

// ----------- OLED SETUP -----------
void setupOLED() {
  Wire.begin(OLED_SDA, OLED_SCL);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "Booting...");
  display.display();
}

// ----------- WIFI SETUP -----------
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  display.clear();
  display.drawString(0, 0, "Connecting WiFi");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  display.clear();
  display.drawString(0, 0,  "WiFi OK:");
  display.drawString(0, 12, WiFi.localIP().toString());
  display.display();
  delay(1000);
}

// ----------- VOLUME CALC FOR OLED -----------
int getVolume(int32_t *samples, int sampleCount) {
  uint32_t sumAbs = 0;
  for (int i = 0; i < sampleCount; i++) {
    int32_t s = samples[i] >> 8;
    if (s < 0) s = -s;
    sumAbs += s;
  }
  uint32_t avg = sumAbs / sampleCount;
  int level = (avg * 100) / 50000;  // tune denominator if needed
  if (level > 100) level = 100;
  return level;
}

void drawBar(int level) {
  display.clear();
  display.drawString(0, 0, "Mic level:");

  int barWidth  = map(level, 0, 100, 0, 120);
  int barHeight = 16;
  int x = 4;
  int y = 20;
  display.drawRect(x, y, 120, barHeight);
  display.fillRect(x, y, barWidth, barHeight);

  display.drawString(0, 40, String(level) + "%");
  display.display();
}

// ----------- GLOBAL BUFFERS -----------
int32_t i2sBuffer[SAMPLES_READ];
int16_t pcmBuffer[SAMPLES_READ];

void setup() {
  Serial.begin(115200);
  delay(500);

  setupOLED();
  setupI2S();
  setupWiFi();

  display.clear();
  display.drawString(0, 0, "UDP Mic Ready");
  display.display();
}

void loop() {
  size_t bytesRead = 0;

  esp_err_t result = i2s_read(
    I2S_PORT,
    (void*)i2sBuffer,
    sizeof(i2sBuffer),
    &bytesRead,
    portMAX_DELAY
  );
  if (result != ESP_OK || bytesRead == 0) return;

  int sampleCount = bytesRead / sizeof(int32_t);
  if (sampleCount > SAMPLES_READ) sampleCount = SAMPLES_READ;

  // Convert to 16-bit PCM and stronger high-pass to reduce DC/rumble
  static int32_t prev = 0;
  for (int i = 0; i < sampleCount; i++) {
    int32_t s  = i2sBuffer[i] >> 11;    // input scale
    int32_t hp = s - prev + (prev >> 6); // stronger HPF (>>6)
    prev = s;

    if (hp >  32767)  hp =  32767;
    if (hp < -32768)  hp = -32768;
    pcmBuffer[i] = (int16_t)hp;
  }

  // ----- Noise gate: compute RMS and zero small blocks -----
  uint32_t sumSq = 0;
  for (int i = 0; i < sampleCount; i++) {
    int32_t s = pcmBuffer[i];
    sumSq += (uint32_t)((int32_t)s * (int32_t)s);
  }
  uint32_t rms = sqrtf((float)sumSq / sampleCount);

  // Threshold: adjust 500-1500 depending on your room
  if (rms < 800) {
    for (int i = 0; i < sampleCount; i++) {
      pcmBuffer[i] = 0;
    }
  }

  // Update OLED volume (still uses raw i2sBuffer)
  int vol = getVolume(i2sBuffer, sampleCount);
  drawBar(vol);

  // Send over UDP
  udp.beginPacket(udpAddress, udpPort);
  udp.write((uint8_t*)pcmBuffer, sampleCount * sizeof(int16_t));
  udp.endPacket();
}