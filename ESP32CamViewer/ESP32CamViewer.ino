/*
 * require libraries:
 * https://github.com/moononournation/Arduino_GFX.git
 * https://github.com/bitbank2/JPEGDEC.git
 */
//#define SSID_NAME "myssid"
//#define SSID_PASSWORD "mypassword"
//#define URL "http://192.168.2.1/jpg_stream"
#define SSID_NAME "fsbrowserplus"
#define SSID_PASSWORD "PleaseInputYourPasswordHere"
#define URL "http://fsbrowserplus.local/stream"
#define MJPEG_BUFFER_SIZE (320 * 240 * 2 / 4)

#include <WiFi.h>
#include <HTTPClient.h>

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
#define TFT_BRIGHTNESS 128
#define TFT_BL 23
static Arduino_DataBus *bus = new Arduino_ESP32SPI_DMA(19 /* DC */, 5 /* CS */, 22 /* SCK */, 21 /* MOSI */, -1 /* MISO */, VSPI, true);
static Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 18 /* RST */, 1 /* rotation */);

/* MJPEG Video */
#include "MjpegClass.h"
static MjpegClass mjpeg;
static uint8_t *mjpeg_buf;

static int frameRead = 0;
static unsigned long timeUsedHTTP = 0;
static unsigned long timeUsedDecode = 0;
static unsigned long timeUsedDrawMCU = 0;

// pixel drawing callback
static void drawMCU(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  unsigned long startMs = millis();
  gfx->startWrite();
  gfx->writeAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->writeBytes((uint8_t *)pDraw->pPixels, pDraw->iWidth * pDraw->iHeight * 2);
  gfx->endWrite();
  timeUsedDrawMCU += millis() - startMs;
} /* drawMCU() */

void setup()
{
  Serial.begin(115200);

  // Init Video
  gfx->begin();
  gfx->fillScreen(BLACK);

  WiFi.begin(SSID_NAME, SSID_PASSWORD);

#ifdef TFT_BL
  ledcAttachPin(TFT_BL, 1);     // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000, 8);       // 12 kHz PWM, 8-bit resolution
  ledcWrite(1, TFT_BRIGHTNESS); // brightness 0 - 255
#endif

  mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
  if (!mjpeg_buf)
  {
    Serial.println(F("mjpeg_buf malloc failed!"));
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    // wait for WiFi connection
    Serial.println("Waiting for WiFi connection.");
    delay(500);
  }
  else
  {
    HTTPClient http;

    log_i("[HTTP] begin...\n");
    http.begin(URL);

    log_i("[HTTP] GET...\n");
    int httpCode = http.GET();

    log_i("[HTTP] GET... code: %d\n", httpCode);
    // HTTP header has been send and Server response header has been handled
    if (httpCode <= 0)
    {
      log_i("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    else
    {
      if (httpCode != HTTP_CODE_OK)
      {
        log_i("[HTTP] Not OK!\n");
      }
      else
      {
        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();

        mjpeg.setup(stream, mjpeg_buf, drawMCU, true, true);
        while (stream->available())
        {
          frameRead++;
          unsigned long startMs = millis();

          // Read frame
          mjpeg.readMjpegBuf();
          timeUsedHTTP += millis() - startMs;
          startMs = millis();

          // Play video
          mjpeg.drawJpg();
          timeUsedDecode += millis() - startMs;

          if ((frameRead % 100) == 0)
          {
            Serial.printf(
                "HTTP GET: %.1f ms, Decode: %.1f ms, Draw: %.1f ms, (%.1f fps)\n",
                1.0 * timeUsedHTTP / frameRead,
                1.0 * timeUsedDecode / frameRead,
                1.0 * timeUsedDrawMCU / frameRead,
                1000.0 * frameRead / (timeUsedHTTP + timeUsedDecode));
          }
        }
      }
    }
  }
}
