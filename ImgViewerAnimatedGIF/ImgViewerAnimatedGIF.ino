/*******************************************************************************
 * Animated GIF image Viewer
 * This is a simple Animated GIF image viewer exsample
 * Image Source: https://www.pexels.com/video/earth-rotating-video-856356/
 * cropped: x: 598 y: 178 width: 720 height: 720
 * optimized with ezgif.com
 * 
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload Animated GIF file
 *   SPIFFS (ESP8266 / ESP32):
 *     upload SPIFFS data with ESP32 Sketch Data Upload:
 *     ESP8266: https://github.com/esp8266/arduino-esp8266fs-plugin
 *     ESP32: https://github.com/me-no-dev/arduino-esp32fs-plugin
 *   SD:
 *     Most Arduino system built-in support SD file system.
 *     Wio Terminal require extra dependant Libraries:
 *     - Seeed_Arduino_FS: https://github.com/Seeed-Studio/Seeed_Arduino_FS.git
 *     - Seeed_Arduino_SFUD: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD.git
 ******************************************************************************/
#define GIF_FILENAME "/ezgif.com-optimize.gif"

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
#define TFT_BRIGHTNESS 128
#define TFT_BL 23
static Arduino_DataBus *bus = new Arduino_ESP32SPI_DMA(19 /* DC */, 5 /* CS */, 22 /* SCK */, 21 /* MOSI */, -1 /* MISO */, VSPI, true);
static Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 18 /* RST */, 1 /* rotation */);

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(ESP32)
#include <SPIFFS.h>
// #include <SD.h>
#endif

#include "gifdec.h"

void setup()
{
  Serial.begin(115200);

  // Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  // Init SPIFLASH
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI, 4000000UL))
#else
  if (!SPIFFS.begin())
  // if (!SD.begin())
#endif
  {
    Serial.println(F("ERROR: SPIFFS Mount Failed!"));
    gfx->println(F("ERROR: SPIFFS Mount Failed!"));
  }
  else
  {
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    File gifFile = SD.open(GIF_FILENAME);
#else
    File gifFile = SPIFFS.open(GIF_FILENAME);
    // File gifFile = SD.open(GIF_FILENAME);
#endif
    if (!gifFile || gifFile.isDirectory())
    {
      Serial.println(F("ERROR: open gifFile Failed!"));
      gfx->println(F("ERROR: open gifFile Failed!"));
    }
    else
    {
      // read GIF file header
      gd_GIF *gif = gd_open_gif(&gifFile);
      if (!gif)
      {
        Serial.println(F("gd_open_gif() failed!"));
      }
      else
      {
        int32_t s = gif->width * gif->height;
        uint8_t *buf = (uint8_t *)malloc(s);
        if (!buf)
        {
          Serial.println(F("buf malloc failed!"));
        }
        else
        {
          Serial.println(F("GIF video start"));
          gfx->setAddrWindow((gfx->width() - gif->width) / 2, (gfx->height() - gif->height) / 2, gif->width, gif->height);
          int t_fstart, t_delay = 0, t_real_delay, res, delay_until;
          int duration = 0, remain = 0;
          while (1)
          {
            t_fstart = millis();
            t_delay = gif->gce.delay * 10;
            res = gd_get_frame(gif, buf);
            if (res < 0)
            {
              Serial.println(F("ERROR: gd_get_frame() failed!"));
              break;
            }
            else if (res == 0)
            {
              Serial.printf("rewind, duration: %d, remain: %d (%0.1f %%)\n", duration, remain, 100.0 * remain / duration);
              duration = 0;
              remain = 0;
              gd_rewind(gif);
              continue;
            }

            gfx->startWrite();
            gfx->writeIndexedPixels(buf, gif->palette->colors, s);
            gfx->endWrite();

            t_real_delay = t_delay - (millis() - t_fstart);
            duration += t_delay;
            remain += t_real_delay;
            delay_until = millis() + t_real_delay;
            do
            {
              delay(1);
            } while (millis() < delay_until);
          }
          Serial.println(F("GIF video end"));
          Serial.printf("duration: %d, remain: %d (%0.1f %%)\n", duration, remain, 100.0 * remain / duration);
          gd_close_gif(gif);
        }
      }
    }
  }
}

void loop()
{
}
