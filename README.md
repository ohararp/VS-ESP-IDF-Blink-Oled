# VS-ESP-IDF-Blink-Oled

ESP32-S3 rainbow LED blink with a 128x32 SSD1306 OLED display and OTA firmware updates via GitHub Releases, built with ESP-IDF v6.0.

## What it does

- An onboard WS2812 RGB LED cycles through the full color spectrum using HSV-to-RGB conversion
- A 128x32 SSD1306 OLED displays the firmware version, current hue (0-359), RGB values, and a hue position bar
- Color advances 10 degrees per cycle (full rainbow every 36 steps)
- Press Button C (GPIO39) to trigger an over-the-air firmware update from GitHub Releases
- OLED shows OTA progress: WiFi status, download percentage with progress bar, and reboot confirmation

## Hardware

| Component | Details |
|-----------|---------|
| Board | YD-ESP32-S3 N16R8 (DevKitC clone, 16MB flash) |
| RGB LED | WS2812 on GPIO 48 (onboard) |
| OLED | Adafruit 128x32 SSD1306 FeatherWing (I2C, 0x3C) |
| OTA Button | FeatherWing Button C on GPIO 39 (active-low) |

### Wiring

| OLED Pin | ESP32-S3 GPIO |
|----------|---------------|
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| VCC | 3.3V |
| GND | GND |

I2C pins are configurable via `idf.py menuconfig` under **Example Configuration**.

## Build & Flash

Requires the [VS Code ESP-IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) or ESP-IDF v6.0 CLI.

1. Open the project in VS Code
2. Select the ESP32-S3 target
3. Build and flash using the ESP-IDF extension toolbar

### Configuration

Run `idf.py menuconfig` or use the SDK Configuration Editor to adjust:

- **Blink GPIO** — default 48
- **Blink period** — default 1000 ms
- **LED strip backend** — RMT (default) or SPI
- **OLED I2C SDA/SCL** — default GPIO 8 / GPIO 9
- **OTA WiFi SSID/Password** — WiFi credentials for OTA updates
- **OTA Firmware URL** — URL to download firmware binary (default: this repo's latest GitHub Release)
- **OTA Button GPIO** — default 39

## OTA Firmware Updates

The device can update its own firmware over WiFi by downloading a binary from GitHub Releases.

### How it works

1. Press Button C (GPIO39) on the FeatherWing
2. The OLED shows "OTA UPDATE" and connects to WiFi
3. Firmware is downloaded from the configured GitHub Release URL over HTTPS
4. Download progress is shown on the OLED with a progress bar
5. On success, the device reboots into the new firmware
6. On failure, the OLED shows an error for 5 seconds and returns to normal operation

### Publishing a new firmware release

1. Update the version in `CMakeLists.txt`: `set(PROJECT_VER "x.y.z")`
2. Build the project (output at `build/blink.bin`)
3. Create a GitHub Release with a tag (e.g. `v1.1.0`) and upload `blink.bin`
4. Devices can now pull the update by pressing Button C

### Partition table

The project uses ESP-IDF's `TWO_OTA` partition scheme with 16MB flash:

| Partition | Type | Size |
|-----------|------|------|
| nvs | data | 16KB |
| otadata | data | 8KB |
| phy_init | data | 4KB |
| factory | app | 1MB |
| ota_0 | app | 1MB |
| ota_1 | app | 1MB |

## Project Structure

```
├── CMakeLists.txt
├── sdkconfig.defaults
└── main/
    ├── CMakeLists.txt
    ├── Kconfig.projbuild
    ├── idf_component.yml
    ├── blink_example_main.c
    ├── led_strip_ctrl.h
    ├── led_strip_ctrl.c
    ├── oled_display.h
    ├── oled_display.c
    ├── ota_update.h
    └── ota_update.c
```

## Dependencies

Managed via the [ESP Component Registry](https://components.espressif.com/):

- `espressif/led_strip` ^3.0.0
- `espressif/ssd1306` ^1.0.0
