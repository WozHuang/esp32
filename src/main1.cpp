#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ---------- User configuration ----------
const char* WIFI_SSID = "huang602";
const char* WIFI_PASSWORD = "huang81879628";

// ---------- TFT wiring ----------
constexpr int8_t TFT_CS = 1;
constexpr int8_t TFT_RST = 2;
constexpr int8_t TFT_DC = 42;
constexpr int8_t TFT_MOSI = 41;
constexpr int8_t TFT_SCLK = 40;
constexpr int8_t TFT_MISO = -1;

constexpr long GMT_OFFSET_SECONDS = 8 * 60 * 60;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
constexpr uint32_t CLOCK_REFRESH_MS = 1000;

constexpr int16_t TIME_AREA_X = 32;
constexpr int16_t TIME_AREA_Y = 65;
constexpr int16_t TIME_AREA_W = 256;
constexpr int16_t TIME_AREA_H = 42;
constexpr int16_t DATE_AREA_X = 92;
constexpr int16_t DATE_AREA_Y = 145;
constexpr int16_t DATE_AREA_W = 136;
constexpr int16_t DATE_AREA_H = 20;
constexpr int16_t STATUS_AREA_X = 20;
constexpr int16_t STATUS_AREA_Y = 20;
constexpr int16_t STATUS_AREA_W = 280;
constexpr int16_t STATUS_AREA_H = 24;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

uint32_t lastClockRefresh = 0;
bool timeHasSynchronized = false;

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
  tft.setCursor(40, 68);
  tft.print(timeText);

  tft.fillRect(DATE_AREA_X, DATE_AREA_Y, DATE_AREA_W, DATE_AREA_H,
               ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(100, 148);
  tft.print(dateText);

  Serial.printf("Clock: %s %s\n", dateText, timeText);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32-S3 NTP clock starting");

  initDisplay();
  connectWiFi();
  initTime();
  drawClock();
  lastClockRefresh = millis();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastClockRefresh >= CLOCK_REFRESH_MS) {
    lastClockRefresh = now;
    drawClock();
  }

  delay(10);
}
