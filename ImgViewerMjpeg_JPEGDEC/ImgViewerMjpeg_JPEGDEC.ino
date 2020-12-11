/*******************************************************************************
   Motion JPEG Viewer
   This is a simple Motion JPEG image viewer example
   Image Source: https://github.blog/2014-11-24-from-sticker-to-sculpture-the-making-of-the-octocat-figurine/
   ffmpeg -i Earth.gif -vf "scale=240:240:flags=lanczos" -pix_fmt yuv420p earth.mjpeg

   Dependent libraries:
   JPEGDEC: https://github.com/bitbank2/JPEGDEC.git

   Setup steps:
   1. Change your LCD parameters in Arduino_GFX setting
   2. upload SPIFFS data with ESP32 Sketch Data Upload:
      ESP8266: https://github.com/esp8266/arduino-esp8266fs-plugin
      ESP32: https://github.com/me-no-dev/arduino-esp32fs-plugin
 ******************************************************************************/
#define MJPEG_FILENAME "/earth.mjpeg"
#define FPS 15
#define MJPEG_BUFFER_SIZE (240 * 240 * 2 / 10)

/* Arduino_GFX */
#include "Arduino_GFX_Library.h"
#define TFT_BRIGHTNESS 128
#define TFT_BL 23
Arduino_DataBus *bus = new Arduino_ESP32SPI(19 /* DC */, 5 /* CS */, 22 /* SCK */, 21 /* MOSI */, -1 /* MISO */, VSPI);
Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 18 /* RST */, 1 /* rotation */);

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(ESP32)
#include <SPIFFS.h>
// #include <SD.h>
#endif

#include "MjpegClass.h"
MjpegClass mjpeg;

/* variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long total_read_video = 0;
static unsigned long total_decode_video = 0;
static unsigned long total_show_video = 0;
static unsigned long total_remain = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;

// pixel drawing callback
static void drawMCU(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
}

void setup()
{
  Serial.begin(115200);

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
    File vFile = SD.open(MJPEG_FILENAME);
#else
    File vFile = SPIFFS.open(MJPEG_FILENAME);
    // File vFile = SD.open(MJPEG_FILENAME);
#endif

    if (!vFile || vFile.isDirectory())
    {
      Serial.println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
      gfx->println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
    }
    else
    {
      uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
      if (!mjpeg_buf)
      {
        Serial.println(F("mjpeg_buf malloc failed!"));
      }
      else
      {
        Serial.println(F("PCM audio MJPEG video start"));

        start_ms = millis();
        curr_ms = millis();
        mjpeg.setup(&vFile, mjpeg_buf, drawMCU, false, false);
        next_frame_ms = start_ms + (++next_frame * 1000 / FPS);

        while (vFile.available())
        {
          // Read video
          mjpeg.readMjpegBuf();
          total_read_video += millis() - curr_ms;
          curr_ms = millis();

          if (millis() < next_frame_ms) // check show frame or skip frame
          {
            // Play video
            mjpeg.drawJpg();
            total_decode_video += millis() - curr_ms;

            int remain_ms = next_frame_ms - millis();
            if (remain_ms > 0)
            {
              total_remain += remain_ms;
              delay(remain_ms);
            }
          }
          else
          {
            ++skipped_frames;
            Serial.println(F("Skip frame"));
          }

          curr_ms = millis();
          next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
        }
        int time_used = millis() - start_ms;
        int total_frames = next_frame - 1;
        Serial.println(F("MJPEG end"));
        vFile.close();
        int played_frames = total_frames - skipped_frames;
        float fps = 1000.0 * played_frames / time_used;
        Serial.printf("Played frames: %d\n", played_frames);
        Serial.printf("Skipped frames: %d (%0.1f %%)\n", skipped_frames, 100.0 * skipped_frames / total_frames);
        Serial.printf("Time used: %d ms\n", time_used);
        Serial.printf("Expected FPS: %d\n", FPS);
        Serial.printf("Actual FPS: %0.1f\n", fps);
        Serial.printf("SDMMC read MJPEG: %d ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
        Serial.printf("Decode video: %d ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
        Serial.printf("Show video: %d ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
        Serial.printf("Remain: %d ms (%0.1f %%)\n", total_remain, 100.0 * total_remain / time_used);
      }
    }
  }
}

void loop()
{
}
