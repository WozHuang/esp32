#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "image_data.h"

#define TFT_CS   1
#define TFT_RST  2
#define TFT_DC   42
#define TFT_MOSI 41
#define TFT_SCLK 40

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  // 图片是 320x180，屏幕是 320x240
  // 上下各留 30px 黑边，保持原图 16:9 比例，不裁剪
  tft.drawRGBBitmap(
    0,
    30,
    imageData,
    IMAGE_WIDTH,
    IMAGE_HEIGHT
  );

  Serial.println("Image displayed");
}

void loop() {
}
