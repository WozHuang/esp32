#include <unity.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "weather.h"

constexpr bool stringsEqual(const char* left, const char* right) {
  return (*left == *right) &&
         (*left == '\0' || stringsEqual(left + 1, right + 1));
}

static_assert(stringsEqual(weatherCodeToText(0), "晴"),
              "Clear weather must use Chinese text");
static_assert(stringsEqual(weatherCodeToText(95), "雷雨"),
              "Thunderstorms must use Chinese text");

void testParsesCompleteWeatherPayload() {
  const char* payload = R"json({
    "current":{"temperature_2m":26.4,"weather_code":1},
    "daily":{"temperature_2m_max":[30.2],"temperature_2m_min":[21.8]}
  })json";
  WeatherData weather = {99.0F, 98.0F, 97.0F, 95, true};

  TEST_ASSERT_TRUE(parseWeatherPayload(payload, weather));
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 26.4F, weather.currentC);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 21.8F, weather.minC);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 30.2F, weather.maxC);
  TEST_ASSERT_EQUAL_INT(1, weather.weatherCode);
  TEST_ASSERT_TRUE(weather.valid);
}

void testRejectsMissingFieldsWithoutOverwritingWeather() {
  const char* payload = R"json({
    "current":{"temperature_2m":26.4,"weather_code":1},
    "daily":{"temperature_2m_max":[30.2]}
  })json";
  WeatherData weather = {25.0F, 20.0F, 31.0F, 2, true};

  TEST_ASSERT_FALSE(parseWeatherPayload(payload, weather));
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 25.0F, weather.currentC);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 20.0F, weather.minC);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 31.0F, weather.maxC);
  TEST_ASSERT_EQUAL_INT(2, weather.weatherCode);
  TEST_ASSERT_TRUE(weather.valid);
}

void testRejectsMalformedAndWrongTypePayloads() {
  WeatherData weather = {};
  TEST_ASSERT_FALSE(parseWeatherPayload("not-json", weather));
  TEST_ASSERT_FALSE(parseWeatherPayload(
      R"json({"current":{"temperature_2m":"warm","weather_code":1},"daily":{"temperature_2m_max":[30],"temperature_2m_min":[22]}})json",
      weather));
}

void testMapsRepresentativeWeatherCodes() {
  TEST_ASSERT_EQUAL_STRING("晴", weatherCodeToText(0));
  TEST_ASSERT_EQUAL_STRING("多云", weatherCodeToText(3));
  TEST_ASSERT_EQUAL_STRING("雾", weatherCodeToText(45));
  TEST_ASSERT_EQUAL_STRING("雨", weatherCodeToText(63));
  TEST_ASSERT_EQUAL_STRING("雪", weatherCodeToText(75));
  TEST_ASSERT_EQUAL_STRING("雷雨", weatherCodeToText(95));
  TEST_ASSERT_EQUAL_STRING("未知", weatherCodeToText(999));
}

void runWeatherTests() {
  UNITY_BEGIN();
  RUN_TEST(testParsesCompleteWeatherPayload);
  RUN_TEST(testRejectsMissingFieldsWithoutOverwritingWeather);
  RUN_TEST(testRejectsMalformedAndWrongTypePayloads);
  RUN_TEST(testMapsRepresentativeWeatherCodes);
  UNITY_END();
}

#ifdef ARDUINO
void setup() {
  delay(2000);
  runWeatherTests();
}

void loop() {
}
#else
int main(int, char**) {
  runWeatherTests();
  return 0;
}
#endif
