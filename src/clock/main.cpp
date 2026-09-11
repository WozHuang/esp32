#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <math.h>

#include "app_config.h"
#include "weather.h"

// ---------- TFT wiring ----------
constexpr int8_t TFT_CS = 9;
constexpr int8_t TFT_RST = 3;
constexpr int8_t TFT_DC = 8;
constexpr int8_t TFT_MOSI = 18;
constexpr int8_t TFT_SCLK = 17;
constexpr int8_t TFT_MISO = 16;

constexpr long GMT_OFFSET_SECONDS = 8 * 60 * 60;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
constexpr uint32_t CLOCK_REFRESH_MS = 1000;
constexpr uint32_t WEATHER_REFRESH_MS = 30UL * 60UL * 1000UL;

constexpr int16_t TIME_AREA_X = 32;
constexpr int16_t TIME_AREA_Y = 50;
constexpr int16_t TIME_AREA_W = 256;
constexpr int16_t TIME_AREA_H = 42;
constexpr int16_t DATE_AREA_X = 92;
constexpr int16_t DATE_AREA_Y = 108;
constexpr int16_t DATE_AREA_W = 136;
constexpr int16_t DATE_AREA_H = 20;
constexpr int16_t STATUS_AREA_X = 20;
constexpr int16_t STATUS_AREA_Y = 20;
constexpr int16_t STATUS_AREA_W = 280;
constexpr int16_t STATUS_AREA_H = 24;
constexpr int16_t WEATHER_AREA_X = 16;
constexpr int16_t WEATHER_AREA_Y = 150;
constexpr int16_t WEATHER_AREA_W = 288;
constexpr int16_t WEATHER_AREA_H = 70;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;

uint32_t lastClockRefresh = 0;
uint32_t lastWeatherRefresh = 0;
bool timeHasSynchronized = false;
WeatherData weather = {};

void drawStatus(const char* message, uint16_t color) {
  tft.fillRect(STATUS_AREA_X, STATUS_AREA_Y, STATUS_AREA_W, STATUS_AREA_H,
               ILI9341_BLACK);
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(STATUS_AREA_X, STATUS_AREA_Y);
  tft.print(message);
}

void initDisplay() {
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  u8g2.begin(tft);
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  drawStatus("Display ready", ILI9341_WHITE);
  Serial.println("TFT initialized");
}

void connectWiFi() {
  drawStatus("Connecting WiFi...", ILI9341_WHITE);
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  drawStatus("WiFi connected", ILI9341_GREEN);
}

void initTime() {
  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS,
             "ntp.aliyun.com", "pool.ntp.org");
  Serial.println("NTP configured (UTC+8); waiting for time sync...");
  drawStatus("Waiting for NTP...", ILI9341_YELLOW);
}

void drawClock() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 50)) {
    drawStatus("Waiting for NTP...", ILI9341_RED);
    Serial.println("Waiting for NTP time synchronization...");
    return;
  }

  if (!timeHasSynchronized) {
    timeHasSynchronized = true;
    drawStatus("Time synchronized", ILI9341_GREEN);
    Serial.println("NTP time synchronized");
  }

  char timeText[9];
  char dateText[11];
  strftime(timeText, sizeof(timeText), "%H:%M:%S", &timeInfo);
  strftime(dateText, sizeof(dateText), "%Y-%m-%d", &timeInfo);

  tft.fillRect(TIME_AREA_X, TIME_AREA_Y, TIME_AREA_W, TIME_AREA_H,
               ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(5);
  tft.setCursor(40, 53);
  tft.print(timeText);

  tft.fillRect(DATE_AREA_X, DATE_AREA_Y, DATE_AREA_W, DATE_AREA_H,
               ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(100, 111);
  tft.print(dateText);

  Serial.printf("Clock: %s %s\n", dateText, timeText);
}

void drawWeather() {
  tft.fillRect(WEATHER_AREA_X, WEATHER_AREA_Y, WEATHER_AREA_W,
               WEATHER_AREA_H, ILI9341_BLACK);
  if (!weather.valid) {
    u8g2.setForegroundColor(ILI9341_DARKGREY);
    u8g2.setCursor(WEATHER_AREA_X, WEATHER_AREA_Y + 26);
    u8g2.print("天气不可用");
    return;
  }

  u8g2.setForegroundColor(ILI9341_CYAN);
  u8g2.setCursor(WEATHER_AREA_X, WEATHER_AREA_Y + 16);
  u8g2.print(WEATHER_CITY);
  u8g2.print("  ");
  u8g2.print(weatherCodeToText(weather.weatherCode));

  char temperatureText[64];
  snprintf(temperatureText, sizeof(temperatureText),
           "当前 %d℃  最低 %d℃  最高 %d℃", lroundf(weather.currentC),
           lroundf(weather.minC), lroundf(weather.maxC));
  u8g2.setForegroundColor(ILI9341_WHITE);
  u8g2.setCursor(WEATHER_AREA_X, WEATHER_AREA_Y + 50);
  u8g2.print(temperatureText);
}

void updateWeather() {
  drawStatus("Updating weather...", ILI9341_YELLOW);
  if (fetchWeather(weather)) {
    drawWeather();
    drawStatus("Weather updated", ILI9341_GREEN);
    Serial.printf("Weather: %s %.1fC, low %.1fC, high %.1fC\n",
                  weatherCodeToText(weather.weatherCode), weather.currentC,
                  weather.minC, weather.maxC);
    return;
  }

  drawWeather();
  drawStatus("Weather offline", ILI9341_RED);
  Serial.println("Weather update failed; keeping previous data");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32-S3 NTP clock starting");

  initDisplay();
  connectWiFi();
  initTime();
  drawClock();
  updateWeather();
  lastClockRefresh = millis();
  lastWeatherRefresh = millis();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastClockRefresh >= CLOCK_REFRESH_MS) {
    lastClockRefresh = now;
    drawClock();
  }

  if (now - lastWeatherRefresh >= WEATHER_REFRESH_MS) {
    lastWeatherRefresh = now;
    updateWeather();
  }

  delay(10);
}
