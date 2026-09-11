#pragma once

struct WeatherData {
  float currentC;
  float minC;
  float maxC;
  int weatherCode;
  bool valid;
};

bool parseWeatherPayload(const char* json, WeatherData& output);
constexpr const char* weatherCodeToText(int weatherCode) {
  return weatherCode == 0 ? "晴"
         : weatherCode >= 1 && weatherCode <= 3 ? "多云"
         : weatherCode == 45 || weatherCode == 48 ? "雾"
         : (weatherCode >= 51 && weatherCode <= 67) ||
                   (weatherCode >= 80 && weatherCode <= 82)
             ? "雨"
         : (weatherCode >= 71 && weatherCode <= 77) ||
                   (weatherCode >= 85 && weatherCode <= 86)
             ? "雪"
         : weatherCode >= 95 && weatherCode <= 99 ? "雷雨"
                                                   : "未知";
}
bool fetchWeather(WeatherData& output);
