# ESP32 Study

## Clock configuration

Before building the physical-board clock for the first time, copy
`src/clock/local_config.example.h` to `src/clock/local_config.h` and set the
local Wi-Fi credentials. The local file is ignored by Git. Weather location
defaults to Guangzhou and can be changed in the same file.

The clock retrieves current conditions and the day's minimum and maximum
temperature from [Open-Meteo](https://open-meteo.com/).
