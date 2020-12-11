/*******************************************************************************
 * JPEG Viewer
 * This is a simple JPEG image viewer example
 * Image Source: https://github.blog/2014-11-24-from-sticker-to-sculpture-the-making-of-the-octocat-figurine/
 * 
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 * 
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. upload SPIFFS data with ESP32 Sketch Data Upload:
 *    ESP8266: https://github.com/esp8266/arduino-esp8266fs-plugin
 *    ESP32: https://github.com/me-no-dev/arduino-esp32fs-plugin
 ******************************************************************************/
#define JPEG_FILENAME "/octocat.jpg"

/* Arduino_GFX */
#include "Arduino_GFX_Library.h"
#define TFT_BRIGHTNESS 128
#define TFT_BL 23
static Arduino_DataBus *bus = new Arduino_ESP32SPI_DMA(19 /* DC */, 5 /* CS */, 22 /* SCK */, 21 /* MOSI */, -1 /* MISO */, VSPI, true);
static Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 18 /* RST */, 1 /* rotation */);

#ifdef ESP32
#include <SPIFFS.h>
#endif
#include "JpegClass.h"
static JpegClass jpegClass;

// pixel drawing callback
void jpegDrawCallback(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
}

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
    unsigned long start = millis();

    // read JPEG file header
    jpegClass.draw(
      &SPIFFS, (char *)JPEG_FILENAME, jpegDrawCallback,
      0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);

    Serial.printf("Time used: %d\n", millis() - start);
  }
}

void loop()
{
}
