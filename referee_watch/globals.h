// my_globals.h
#ifndef MY_GLOBALS_H
#define MY_GLOBALS_H

#include <Arduino.h>
#include "Arduino_GFX_Library.h"

extern Arduino_GFX *gfx; // Tells other files this variable exists elsewhere
extern bool rtc_ready;
extern SensorPCF85063 rtc;

#define AUDIO 1

#define I2S_MCLK  19
#define I2S_BCLK  20
#define I2S_LRCK  22
#define I2S_DOUT  23   // ESP32 -> codec DSDIN (speaker)
#define I2S_DIN   21   // codec ASDOUT -> ESP32 (mic, unused)
#define PA_PIN    6

#define SAMPLE_RATE 16000

#if AUDIO
#include <ESP_I2S.h>
#include "es8311.h"
extern I2SClass i2s;
#endif

#endif
