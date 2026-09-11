#pragma once

#ifdef WOKWI
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr char WEATHER_CITY[] = "广州";
constexpr float WEATHER_LATITUDE = 23.1291F;
constexpr float WEATHER_LONGITUDE = 113.2644F;
#else
#include "local_config.h"
#endif
