#include "weather.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "app_config.h"

bool fetchWeather(WeatherData& output) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(WEATHER_LATITUDE, 4);
  url += "&longitude=";
  url += String(WEATHER_LONGITUDE, 4);
  url += "&current=temperature_2m,weather_code";
  url += "&daily=temperature_2m_max,temperature_2m_min";
  url += "&timezone=Asia%2FHong_Kong&forecast_days=1";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  if (!http.begin(client, url)) {
    return false;
  }

  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();
  return parseWeatherPayload(payload.c_str(), output);
}
