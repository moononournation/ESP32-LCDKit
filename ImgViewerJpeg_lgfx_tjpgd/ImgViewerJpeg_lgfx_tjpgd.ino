/*******************************************************************************
 * JPEG Viewer
 * This is a simple JPEG image viewer exsample
 * Image Source: https://github.blog/2014-11-24-from-sticker-to-sculpture-the-making-of-the-octocat-figurine/
 * 
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. upload SPIFFS data with ESP32 Sketch Data Upload:
 *    ESP8266: https://github.com/esp8266/arduino-esp8266fs-plugin
 *    ESP32: https://github.com/me-no-dev/arduino-esp32fs-plugin
 ******************************************************************************/
#define JPEG_FILENAME "/octocat.jpg"

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
#define TFT_BRIGHTNESS 128
#define TFT_BL 23
static Arduino_DataBus *bus = new Arduino_ESP32SPI(19 /* DC */, 5 /* CS */, 22 /* SCK */, 21 /* MOSI */, -1 /* MISO */, VSPI, true);
static Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 18 /* RST */, 1 /* rotation */);

#ifdef ESP32
#include <SPIFFS.h>
#endif
#include "JpegDec.h"
static JpegDec jpegDec;

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

  // Init SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println(F("ERROR: SPIFFS Mount Failed!"));
    gfx->println(F("ERROR: SPIFFS Mount Failed!"));
  }
  else
  {
    File jpegFile = SPIFFS.open(JPEG_FILENAME, "r");
    if (!jpegFile)
    {
      Serial.println(F("ERROR: open jpegFile Failed!"));
      gfx->println(F("ERROR: open jpegFile Failed!"));
    }
    else
    {
      unsigned long start = millis();
      // read JPEG file header
      jpegDec.prepare(jpegDec.file_reader, &jpegFile);

      // scale to fit height
      jpg_scale_t scale;
      float ratio = (float)jpegDec.height / gfx->height();
      if (ratio <= 1)
      {
        scale = JPG_SCALE_NONE;
      }
      else if (ratio <= 2)
      {
        scale = JPG_SCALE_2X;
      }
      else if (ratio <= 4)
      {
        scale = JPG_SCALE_4X;
      }
      else
      {
        scale = JPG_SCALE_8X;
      }

      // decode and output
      jpegDec.decode(scale, jpegDec.gfx_writer, gfx);
      Serial.printf("Time used: %d\n", millis() - start);
    }
  }
}

void loop()
{
}
