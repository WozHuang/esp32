# Repository Guidelines

## 项目结构与模块组织

本仓库是基于 Arduino 框架的 ESP32-S3 PlatformIO 工程。`src/clock/` 包含联网 NTP 时钟应用；`src/image_viewer/` 包含图片显示应用，其中 `image_data.h` 保存屏幕图像数据。每个应用目录只能定义一组 `setup()` 和 `loop()`。硬件及依赖配置位于 `platformio.ini`，Wokwi 电路和固件入口分别位于 `diagram.json` 与 `wokwi.toml`。`.pio/` 是生成目录，不应提交。

## 构建、测试与开发命令

在仓库根目录运行：

```powershell
pio run                         # 构建默认 clock 环境
pio run -e clock                # 构建实体板时钟
pio run -e image-viewer         # 构建实体板图片应用
pio run -e clock-wokwi          # 构建 Wokwi 时钟固件
pio run -e clock -t upload      # 烧录时钟固件
pio device monitor -b 115200    # 打开串口监视器
```

启动 Wokwi 前，先构建 `wokwi.toml` 当前指向的环境，再从 VS Code 命令面板运行 `Wokwi: Start Simulator`。

## 编码风格与命名约定

C++ 使用两个空格缩进，左大括号与声明同行。函数采用 lowerCamelCase，例如 `initDisplay()`；常量和引脚采用 `UPPER_SNAKE_CASE`，例如 `TFT_MOSI`。使用 `constexpr` 表达编译期常量，公共配置集中放在文件顶部。新增应用应放入独立的 `src/<app_name>/` 目录，并在 `platformio.ini` 中用 `build_src_filter` 隔离入口代码。避免无关重构。

## 测试指南

天气模块已配置 PlatformIO Unity 测试。天气解析和天气码映射测试位于 `test/test_weather/`：

```powershell
pio test -e weather-test
pio test -e weather-test --without-uploading --without-testing
```

第一条命令用于连接测试板时运行断言；没有连接测试板时，第二条命令只编译测试固件。此时只能声明“测试固件编译成功”，不能声明 Unity 断言已经运行通过。修改公共配置或依赖时应构建所有受影响环境，包括 `clock`、`clock-wokwi` 和 `image-viewer`；修改 TFT 布局时还应在 Wokwi 或实体屏上检查方向、颜色、文字、缺字、越界和刷新残影。提交前确认对应目录生成了 `firmware.bin` 和 `firmware.elf`，且没有重复定义 `setup()` 或 `loop()`。

## 时钟天气模块

`src/clock/weather_data.cpp` 负责解析 Open-Meteo JSON 和天气码映射；`src/clock/weather_client.cpp` 负责 HTTPS 请求。解析函数必须先完整验证响应，成功后才能覆盖已有 `WeatherData`，确保请求失败时保留上一次天气。

天气服务固定使用 Open-Meteo，默认位置为广州，时区使用 `Asia/Hong_Kong`。天气每 30 分钟更新一次，不得阻塞每秒时钟刷新。

## 本地配置

实体板配置存放在被 Git 忽略的 `src/clock/local_config.h`。首次构建前从 `local_config.example.h` 复制创建。禁止提交真实 Wi-Fi SSID、密码或其他凭据。Wokwi 使用 `Wokwi-GUEST`，不得依赖本地配置文件。

## 中文显示

天气区域使用 `U8g2_for_Adafruit_GFX` 和 `u8g2_font_wqy16_t_gb2312` 显示 UTF-8 中文。Adafruit GFX 默认字体不能直接显示中文。

## 安全与硬件配置

不要提交真实 Wi-Fi 密码；使用本地占位值或未跟踪配置。修改 GPIO、Flash、PSRAM 或分区参数时，同时核对实体接线和 `diagram.json`。仿真专用行为必须通过独立环境或条件编译隔离，确保实体板构建不受影响。
