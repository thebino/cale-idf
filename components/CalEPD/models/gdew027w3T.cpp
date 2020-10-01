#include "gdew027w3T.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/task.h"

// This class is refactored to cope with Good display Arduino example


DRAM_ATTR const epd_init_3 Gdew027w3T::epd_soft_start={
0x06,{0x07,0x07,0x17},3
};

DRAM_ATTR const epd_init_1 Gdew027w3T::epd_extra_setting={
0x16,{0x00},1
};
// LUT from OTP 128x296
DRAM_ATTR const epd_init_1 Gdew027w3T::epd_panel_setting={
0x00,{0x1f},1
};

DRAM_ATTR const epd_init_1 Gdew027w3T::epd_vcom2={
0x50,{0x97},1
};

// Constructor
Gdew027w3T::Gdew027w3T(EpdSpi& dio): 
  Adafruit_GFX(GDEW027W3_WIDTH, GDEW027W3_HEIGHT),
  Epd(GDEW027W3_WIDTH, GDEW027W3_HEIGHT), IO(dio)
{
  printf("Gdew027w3T() %d*%d\n",
  GDEW027W3_WIDTH, GDEW027W3_HEIGHT);  
  // For the record, begining of the fight: https://twitter.com/martinfasani/status/1265762052880175107
}

//Initialize the display
void Gdew027w3T::init(bool debug)
{
    debug_enabled = debug;
    if (debug_enabled) printf("Gdew027w3T::init(%d)\n", debug);
    IO.init(5, debug); // 5 MHz frequency

    printf("Free heap:%d\n",xPortGetFreeHeapSize());
    fillScreen(EPD_WHITE);
}

void Gdew027w3T::fillScreen(uint16_t color)
{
  // 0xFF = 8 pixels black, 0x00 = 8 pix. white
  uint8_t data = (color == EPD_BLACK) ? GDEW027W3_8PIX_BLACK : GDEW027W3_8PIX_WHITE;
  for (uint16_t x = 0; x < sizeof(_buffer); x++)
  {
    _buffer[x] = data;
  }

  if (debug_enabled) printf("fillScreen(%d) _buffer len:%d\n",data,sizeof(_buffer));
}

void Gdew027w3T::_wakeUp(){
  printf("wakeup() start commands\n");
  IO.reset(10);


  //KW-BF   KWR-AF  BWROTP 0f
  IO.cmd(epd_soft_start.cmd);     // boost
  for (int i=0;i<epd_soft_start.databytes;++i) {
      IO.data(epd_soft_start.data[i]);
  }

  IO.cmd(0x04);
  _waitBusy("epd_wakeup_power:ON");
  _isAsleep = false;

  IO.cmd(epd_extra_setting.cmd);  // CMD: 0x16 DATA: 0x00
  IO.data(epd_extra_setting.data[0]);
  
  IO.cmd(epd_panel_setting.cmd);  // CMD: 0x00 DATA: 0xbf
  IO.data(epd_panel_setting.data[0]);

  IO.cmd(0xF8);         //boost 1
  IO.data(0x60);
  IO.data(0xa5);

  IO.cmd(0xF8);         //boost 2
  IO.data(0x73);
  IO.data(0x23);

  IO.cmd(0xF8);         //boost 3
  IO.data(0x7C);
  IO.data(0x00);

  vTaskDelay(2/portTICK_RATE_MS); // delay(2)

  //WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
  IO.cmd(epd_vcom2.cmd);          // vcom and data interval
  IO.data(epd_vcom2.data[0]);     // CMD: 0x50 DATA: 0x97 
}

void Gdew027w3T::update()
{
  _wakeUp();
  _using_partial_mode = false;

  IO.cmd(0x13);        // update current data
  for (uint16_t x = 0; x < GDEW027W3_BUFFER_SIZE; x++){
    if (debug_enabled){
      if (x < 40) {
        printf("%x ",_buffer[x]);
      }
    }
    uint8_t pixel = sizeof(_buffer) ? ~_buffer[x] : 0xFF;
    IO.data(pixel);
  } 

  IO.cmd(0x12);        // display refresh
  _waitBusy("update");

  if (_sleep_after_update) {
    _sleep();
  }
}

uint16_t Gdew027w3T::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye) {
  IO.data(x >> 8);
  IO.data(x & 0xf8);
  IO.data(y >> 8);
  IO.data(y & 0xff);
  IO.data(xe >> 8);
  IO.data(xe & 0xf8);
  IO.data(ye >> 8);
  IO.data(ye & 0xff);
 return 1;
}

void Gdew027w3T::_partialRamArea(uint8_t command, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  IO.cmd(command);
  _setPartialRamArea(x,y,w,h);
}

void Gdew027w3T::_refreshWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  w += (x % 8) + 7;
  h = gx_uint16_min(h, 256); // strange controller error
  IO.cmd(0x16);
  IO.data(x >> 8);
  IO.data(x & 0xf8);
  IO.data(y >> 8);
  IO.data(y & 0xff);
  IO.data(w >> 8);
  IO.data(w & 0xf8);
  IO.data(h >> 8);
  IO.data(h & 0xff);
}

void Gdew027w3T::_writeToWindow(uint8_t command, uint16_t xs, uint16_t ys, uint16_t xd, uint16_t yd, uint16_t w, uint16_t h)
{
  //Serial.printf("_writeToWindow(%d, %d, %d, %d, %d, %d)\n", xs, ys, xd, yd, w, h);
  // the screen limits are the hard limits
  if (xs >= GDEW027W3_WIDTH) return;
  if (ys >= GDEW027W3_HEIGHT) return;
  if (xd >= GDEW027W3_WIDTH) return;
  if (yd >= GDEW027W3_HEIGHT) return;
  w = gx_uint16_min(w + 7, GDEW027W3_WIDTH - xd) + (xd % 8);
  h = gx_uint16_min(h, GDEW027W3_HEIGHT - yd);
  uint16_t xe = (xs / 8) + (w / 8);
  IO.cmd(0x91); // partial in
  _partialRamArea(command, xd, yd, w, h);
  for (uint16_t y1 = ys; y1 < ys + h; y1++)
  {
    for (uint16_t x1 = xs / 8; x1 < xe; x1++)
    {
      uint16_t idx = y1 * (GDEW027W3_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      IO.data(~data);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(2));
}

void Gdew027w3T::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (using_rotation) _rotate(x, y, w, h);
  // Only if sleep state is true:
  if (!_using_partial_mode || _isAsleep==true) _wakeUp();
  _using_partial_mode = true;

  _writeToWindow(0x15, x, y, x, y, w, h);
  _refreshWindow(x, y, w, h);   
  _waitBusy("updateWindow");
  _writeToWindow(0x14, x, y, x, y, w, h);
}

void Gdew027w3T::_waitBusy(const char* message){
  if (debug_enabled) {
    ESP_LOGI(TAG, "_waitBusy for %s", message);
  }
  int64_t time_since_boot = esp_timer_get_time();

  while (1){
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 1) break;
    vTaskDelay(1);
    if (esp_timer_get_time()-time_since_boot>7000000)
    {
      if (debug_enabled) ESP_LOGI(TAG, "Busy Timeout");
      break;
    }
  }
}

void Gdew027w3T::_sleep(){
  _isAsleep = true;
  IO.cmd(0x02);
  _waitBusy("power_off");
  IO.cmd(0x07);
  IO.data(0xa5);
}

void Gdew027w3T::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = GDEW027W3_WIDTH - x - w - 1;
      break;
    case 2:
      x = GDEW027W3_WIDTH - x - w - 1;
      y = GDEW027W3_HEIGHT - y - h - 1;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = GDEW027W3_HEIGHT - y - h - 1;
      break;
  }
}


void Gdew027w3T::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = GDEW027W3_WIDTH - x - 1;
      break;
    case 2:
      x = GDEW027W3_WIDTH - x - 1;
      y = GDEW027W3_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = GDEW027W3_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * GDEW027W3_WIDTH / 8;

  // This is the trick to draw colors right. Genious Jean-Marc
  if (color) {
    _buffer[i] = (_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
    } else {
    _buffer[i] = (_buffer[i] | (1 << (7 - x % 8)));
    }
}
