#include "weather.h"

#include <ArduinoJson.h>

bool parseWeatherPayload(const char* json, WeatherData& output) {
  JsonDocument document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return false;
  }

  JsonVariantConst currentC = document["current"]["temperature_2m"];
  JsonVariantConst weatherCode = document["current"]["weather_code"];
  JsonVariantConst maxC = document["daily"]["temperature_2m_max"][0];
  JsonVariantConst minC = document["daily"]["temperature_2m_min"][0];
  if (!currentC.is<float>() || !weatherCode.is<int>() || !maxC.is<float>() ||
      !minC.is<float>()) {
    return false;
  }

  WeatherData parsed = {
      currentC.as<float>(), minC.as<float>(), maxC.as<float>(),
      weatherCode.as<int>(), true};
  output = parsed;
  return true;
}
