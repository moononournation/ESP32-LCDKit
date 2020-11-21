#ifndef _MJPEGCLASS_H_
#define _MJPEGCLASS_H_

#define READ_BUFFER_SIZE 1024
#define NUMBER_OF_DRAW_BUFFER 4

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_heap_caps.h>
#include <FS.h>

#include <JPEGDEC.h>

typedef struct
{
  JPEG_DRAW_CALLBACK *drawFunc;
} param_task_output;

static xQueueHandle xqh = 0;
static JPEGDRAW jpegdraws[NUMBER_OF_DRAW_BUFFER];
static int queue_cnt = 0;
static int draw_cnt = 0;

static void queueDrawMCU(JPEGDRAW *pDraw)
{
  ++queue_cnt;
  while ((queue_cnt - draw_cnt) > NUMBER_OF_DRAW_BUFFER)
  {
    delay(1);
  }

  int len = pDraw->iWidth * pDraw->iHeight * 2;
  JPEGDRAW *j = &jpegdraws[queue_cnt % NUMBER_OF_DRAW_BUFFER];
  j->x = pDraw->x;
  j->y = pDraw->y;
  j->iWidth = pDraw->iWidth;
  j->iHeight = pDraw->iHeight;
  memcpy(j->pPixels, pDraw->pPixels, len);

  xQueueSend(xqh, &j, 0);

  // xQueueSend(xqh, &pDraw, 0);
}

static void drawTask(void *arg)
{
  param_task_output *p = (param_task_output *)arg;
  for (int i = 0; i < NUMBER_OF_DRAW_BUFFER; i++)
  {
    jpegdraws[i].pPixels = (uint16_t *)heap_caps_malloc(4 * 16 * 16 * 2, MALLOC_CAP_DMA);
  }
  JPEGDRAW *pDraw;
  Serial.println("drawTask start");
  while (xQueueReceive(xqh, &pDraw, portMAX_DELAY))
  {
    // Serial.printf("task work: x: %d, y: %d, iWidth: %d, iHeight: %d\r\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    p->drawFunc(pDraw);
    // Serial.println("task work done");
    ++draw_cnt;
  }
  vQueueDelete(xqh);
  Serial.println("drawTask end");
  vTaskDelete(NULL);
}

class MjpegClass
{
public:
  bool setup(File input, uint8_t *mjpeg_buf, JPEG_DRAW_CALLBACK *pfnDraw)
  {
    _input = input;
    _mjpeg_buf = mjpeg_buf;

    _tft_width = gfx->width();
    _tft_height = gfx->height();

    _read_buf = (uint8_t *)malloc(READ_BUFFER_SIZE);

    TaskHandle_t task;
    _p.drawFunc = pfnDraw;
    xqh = xQueueCreate(NUMBER_OF_DRAW_BUFFER, sizeof(JPEGDRAW));
    xTaskCreatePinnedToCore(drawTask, "drawTask", 1600, &_p, 1, &task, 0);

    return true;
  }

  bool readMjpegBuf()
  {
    if (_inputindex == 0)
    {
      _buf_read = _input.read(_read_buf, READ_BUFFER_SIZE);
      _inputindex += _buf_read;
    }
    _mjpeg_buf_offset = 0;
    int i = 3;
    bool found_FFD9 = false;
    if (_buf_read > 0)
    {
      i = 3;
      while ((_buf_read > 0) && (!found_FFD9))
      {
        if ((_mjpeg_buf_offset > 0) && (_mjpeg_buf[_mjpeg_buf_offset - 1] == 0xFF) && (_read_buf[0] == 0xD9)) // JPEG trailer
        {
          found_FFD9 = true;
        }
        else
        {
          while ((i < _buf_read) && (!found_FFD9))
          {
            if ((_read_buf[i] == 0xFF) && (_read_buf[i + 1] == 0xD9)) // JPEG trailer
            {
              found_FFD9 = true;
              ++i;
            }
            ++i;
          }
        }

        // Serial.printf("i: %d\n", i);
        memcpy(_mjpeg_buf + _mjpeg_buf_offset, _read_buf, i);
        _mjpeg_buf_offset += i;
        size_t o = _buf_read - i;
        if (o > 0)
        {
          // Serial.printf("o: %d\n", o);
          memcpy(_read_buf, _read_buf + i, o);
          _buf_read = _input.read(_read_buf + o, READ_BUFFER_SIZE - o);
          _inputindex += _buf_read;
          _buf_read += o;
          // Serial.printf("_buf_read: %d\n", _buf_read);
        }
        else
        {
          _buf_read = _input.read(_read_buf, READ_BUFFER_SIZE);
          _inputindex += _buf_read;
        }
        i = 0;
      }
      if (found_FFD9)
      {
        return true;
      }
    }

    return false;
  }

  bool drawJpg()
  {
    _fileindex = 0;
    _remain = _mjpeg_buf_offset;

    _jpeg.openRAM(_mjpeg_buf, _remain, queueDrawMCU);

    int w = _jpeg.getWidth();
    int h = _jpeg.getHeight();

    _jpeg.setMaxOutputSize(4);
    _jpeg.setPixelType(RGB565_BIG_ENDIAN);
    _jpeg.decode(0, 0, 0);
    _jpeg.close();

    return true;
  }

private:
  File _input;
  uint8_t *_read_buf;
  uint8_t *_mjpeg_buf;
  int32_t _mjpeg_buf_offset = 0;

  JPEGDEC _jpeg;
  param_task_output _p;

  int32_t _inputindex = 0;
  int32_t _buf_read;
  int32_t _remain = 0;
  uint32_t _fileindex;

  int32_t _tft_width;
  int32_t _tft_height;
};

#endif // _MJPEGCLASS_H_
