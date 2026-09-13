#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <XPT2046_Touchscreen.h>
#include <esp_system.h>

#include "answers.h"

// TFT wiring (same as the clock application).
constexpr int8_t TFT_CS = 9;
constexpr int8_t TFT_RST = 3;
constexpr int8_t TFT_DC = 8;
constexpr int8_t TFT_MOSI = 18;
constexpr int8_t TFT_SCLK = 17;
constexpr int8_t TFT_MISO = 16;

// XPT2046 wiring. T_CS was moved from GPIO 8 to GPIO 7 to avoid TFT_DC.
constexpr int8_t TOUCH_IRQ = 4;
constexpr int8_t TOUCH_MISO = 5;
constexpr int8_t TOUCH_MOSI = 6;
constexpr int8_t TOUCH_CS = 7;
constexpr int8_t TOUCH_SCLK = 15;

constexpr int16_t SCREEN_WIDTH = 320;
constexpr int16_t SCREEN_HEIGHT = 240;
constexpr uint32_t LOADING_DURATION_MS = 1666;

// Adjust these values using the raw coordinates printed to Serial if needed.
constexpr int16_t TOUCH_MIN_X = 450;
constexpr int16_t TOUCH_MAX_X = 3800;
constexpr int16_t TOUCH_MIN_Y = 320;
constexpr int16_t TOUCH_MAX_Y = 3800;
constexpr bool TOUCH_SWAP_XY = false;
constexpr bool TOUCH_INVERT_X = true;
constexpr bool TOUCH_INVERT_Y = true;

constexpr int16_t BUTTON_X = 45;
constexpr int16_t BUTTON_Y = 174;
constexpr int16_t BUTTON_WIDTH = 230;
constexpr int16_t BUTTON_HEIGHT = 48;
constexpr int16_t BUTTON_RADIUS = 10;

enum class AppState {
  HOME,
  LOADING,
  ANSWER
};

struct TouchPoint {
  int16_t x;
  int16_t y;
};

SPIClass displaySpi(HSPI);
Adafruit_ILI9341 tft(&displaySpi, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
U8G2_FOR_ADAFRUIT_GFX u8g2;

AppState appState = AppState::HOME;
uint32_t loadingStartedAt = 0;
const char* selectedAnswer = nullptr;
bool touchWasDown = false;

void drawCenteredText(const char* text, int16_t baseline, uint16_t color) {
  u8g2.setForegroundColor(color);
  const int16_t width = u8g2.getUTF8Width(text);
  const int16_t x = max<int16_t>(0, (SCREEN_WIDTH - width) / 2);
  u8g2.setCursor(x, baseline);
  u8g2.print(text);
}

void drawScreenBorder() {
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_YELLOW);
  tft.drawRect(1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2, ILI9341_YELLOW);
}

void drawButton(const char* label) {
  tft.fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
                    BUTTON_RADIUS, ILI9341_DARKCYAN);
  tft.drawRoundRect(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
                    BUTTON_RADIUS, ILI9341_CYAN);
  drawCenteredText(label, BUTTON_Y + 31, ILI9341_WHITE);
}

void drawHomePage() {
  tft.fillScreen(ILI9341_BLACK);
  drawScreenBorder();
  drawCenteredText("答案之书", 42, ILI9341_YELLOW);
  drawCenteredText("在心里想一个问题，", 92, ILI9341_WHITE);
  drawCenteredText("然后获取答案。", 122, ILI9341_WHITE);
  drawButton("获取答案");
}

void drawLoadingPage() {
  tft.fillScreen(ILI9341_BLACK);
  drawScreenBorder();
  drawCenteredText("正在寻找答案……", 126, ILI9341_CYAN);
}

void drawAnswerPage(const char* answer) {
  tft.fillScreen(ILI9341_BLACK);
  drawScreenBorder();
  drawCenteredText("答案是", 42, ILI9341_YELLOW);
  drawCenteredText(answer, 118, ILI9341_WHITE);
  drawButton("再问一次");
}

void initDisplay() {
  displaySpi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  u8g2.begin(tft);
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
}

void initTouch() {
  SPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin();
  touch.setRotation(1);
}

int16_t mapTouchAxis(int16_t raw, int16_t minimum, int16_t maximum,
                     int16_t outputMaximum, bool invert) {
  const int16_t bounded = constrain(raw, minimum, maximum);
  int16_t mapped = map(bounded, minimum, maximum, 0, outputMaximum - 1);
  if (invert) {
    mapped = outputMaximum - 1 - mapped;
  }
  return mapped;
}

TouchPoint mapTouchPoint(const TS_Point& raw) {
  int16_t rawX = raw.x;
  int16_t rawY = raw.y;
  if (TOUCH_SWAP_XY) {
    const int16_t temporary = rawX;
    rawX = rawY;
    rawY = temporary;
  }

  return {
      mapTouchAxis(rawX, TOUCH_MIN_X, TOUCH_MAX_X, SCREEN_WIDTH,
                   TOUCH_INVERT_X),
      mapTouchAxis(rawY, TOUCH_MIN_Y, TOUCH_MAX_Y, SCREEN_HEIGHT,
                   TOUCH_INVERT_Y)
  };
}

bool readTouchPress(TouchPoint& point) {
  const bool touchIsDown = touch.touched();
  if (!touchIsDown) {
    touchWasDown = false;
    return false;
  }

  const TS_Point raw = touch.getPoint();
  point = mapTouchPoint(raw);

  if (touchWasDown) {
    return false;
  }
  touchWasDown = true;

  Serial.printf("Touch raw=(%d,%d) mapped=(%d,%d) pressure=%d\n",
                raw.x, raw.y, point.x, point.y, raw.z);
  return true;
}

bool isInsideButton(const TouchPoint& point) {
  return point.x >= BUTTON_X && point.x < BUTTON_X + BUTTON_WIDTH &&
         point.y >= BUTTON_Y && point.y < BUTTON_Y + BUTTON_HEIGHT;
}

const char* selectRandomAnswer() {
  const size_t index = static_cast<size_t>(random(answerCount));
  Serial.printf("Selected answer %u of %u: %s\n",
                static_cast<unsigned>(index),
                static_cast<unsigned>(answerCount), answers[index]);
  return answers[index];
}

void showHome() {
  appState = AppState::HOME;
  selectedAnswer = nullptr;
  drawHomePage();
}

void startLoading() {
  appState = AppState::LOADING;
  loadingStartedAt = millis();
  drawLoadingPage();
}

void showRandomAnswer() {
  selectedAnswer = selectRandomAnswer();
  appState = AppState::ANSWER;
  drawAnswerPage(selectedAnswer);
}

void handleTouchEvent(const TouchPoint& point) {
  if (!isInsideButton(point)) {
    return;
  }

  if (appState == AppState::HOME) {
    startLoading();
  } else if (appState == AppState::ANSWER) {
    showHome();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nOffline answer book starting");

  randomSeed(esp_random());
  initDisplay();
  initTouch();
  showHome();

  Serial.printf("Ready with %u built-in answers\n",
                static_cast<unsigned>(answerCount));
}

void loop() {
  TouchPoint point = {};
  if (readTouchPress(point) && appState != AppState::LOADING) {
    handleTouchEvent(point);
  }

  if (appState == AppState::LOADING &&
      millis() - loadingStartedAt >= LOADING_DURATION_MS) {
    showRandomAnswer();
  }

  delay(10);
}
