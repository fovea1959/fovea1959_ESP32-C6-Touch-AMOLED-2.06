#include <Wire.h>

#define DO 1

#if DO
#include <ESP_I2S.h>
#include "es8311.h"
#endif

#define I2C_SDA   8
#define I2C_SCL   7
#define I2S_MCLK  19
#define I2S_BCLK  20
#define I2S_LRCK  22
#define I2S_DOUT  23   // ESP32 -> codec DSDIN (speaker)
#define I2S_DIN   21   // codec ASDOUT -> ESP32 (mic, unused)
#define PA_PIN    6

#define SAMPLE_RATE 16000

#if DO
I2SClass i2s;
#endif

void enableAmp() {
  pinMode(PA_PIN, OUTPUT);
  digitalWrite(PA_PIN, HIGH);
}

void disableAmp() {
  pinMode(PA_PIN, OUTPUT);
  digitalWrite(PA_PIN, LOW);
}

void scanI2C() {
  Serial.println("I2C scan:");
  for (uint8_t a = 1; a < 127; a++) {
    Serial.printf("  checking 0x%02X: ", a);
    Wire.beginTransmission(a);
    int error = Wire.endTransmission();
    Serial.printf("got 0x%02X", error);
    if (error == 0) {
      Serial.printf(" **present**");
    }
    Serial.printf("\n");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL, 100000);   // safe speed
  delay(500);
  scanI2C();                    // expect 0x18 (the codec)
  // If your es8311 files install the ESP-IDF I2C driver themselves
  // and you get an I2C driver conflict, uncomment:
  //Wire.end();

#if DO
  i2s.setPins(I2S_BCLK, I2S_LRCK, I2S_DOUT, I2S_DIN, I2S_MCLK);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("I2S init failed");
    while (1) delay(1000);
  }

  es8311_handle_t es = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
  const es8311_clock_config_t clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = SAMPLE_RATE * 256,
    .sample_frequency = SAMPLE_RATE
  };
  esp_err_t err = es8311_init(es, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
  Serial.printf("es8311_init: %d (0 = OK)\n", err);
  es8311_voice_volume_set(es, 100, NULL);   // was 70
  es8311_microphone_config(es, false);

#endif
  Serial.println("setup done");
}

void loop() {
#if DO
  static int16_t buf[256 * 2];
  static float phase = 0;
  size_t written = 0;
  enableAmp();
  for (int n = 0; n < SAMPLE_RATE / 256; n++) {
    for (int i = 0; i < 256; i++) {
      int16_t s = (int16_t)(sinf(phase) * 16000);   // was 8000
      phase += 2 * PI * 880.0f / SAMPLE_RATE;       // was 440.0f
      if (phase > 2 * PI) phase -= 2 * PI;
      buf[2 * i] = s;
      buf[2 * i + 1] = s;
    }
    written += i2s.write((uint8_t *)buf, sizeof(buf));
  }
  Serial.printf("wrote %u bytes\n", (unsigned)written);
  disableAmp();
#endif
  delay(1000);
}