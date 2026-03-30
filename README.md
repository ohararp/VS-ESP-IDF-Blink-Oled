# VS-ESP-IDF-Blink-Oled

ESP32-S3 rainbow LED blink with a 128x32 SSD1306 OLED display, built with ESP-IDF v6.0.

## What it does

- An onboard WS2812 RGB LED cycles through the full color spectrum using HSV-to-RGB conversion
- A 128x32 SSD1306 OLED displays the current hue (0–359), RGB values, and a hue position bar
- Color advances 10 degrees per cycle (full rainbow every 36 steps)

## Hardware

| Component | Details |
|-----------|---------|
| Board | YD-ESP32-S3 N16R8 (DevKitC clone) |
| RGB LED | WS2812 on GPIO 48 (onboard) |
| OLED | Adafruit 128x32 SSD1306 FeatherWing (I2C, 0x3C) |

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

## Project Structure

```
├── CMakeLists.txt
├── sdkconfig.defaults
└── main/
    ├── CMakeLists.txt
    ├── Kconfig.projbuild
    ├── idf_component.yml
    └── blink_example_main.c
```

## Dependencies

Managed via the [ESP Component Registry](https://components.espressif.com/):

- `espressif/led_strip` ^3.0.0
- `espressif/ssd1306` ^1.0.0
