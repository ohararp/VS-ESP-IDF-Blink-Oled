/* Blink + OLED Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

/* OLED configuration */
#define OLED_I2C_SDA   CONFIG_OLED_I2C_SDA
#define OLED_I2C_SCL   CONFIG_OLED_I2C_SCL
#define OLED_I2C_ADDR  0x3C
#define OLED_W         128
#define OLED_H         32

static uint16_t s_hue = 0;

/* ───────────────── Minimal 5x7 bitmap font (ASCII 32-126) ───────────────── */

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32 (space) */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33 ! */
    {0x00,0x07,0x00,0x07,0x00}, /* 34 " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35 # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36 $ */
    {0x23,0x13,0x08,0x64,0x62}, /* 37 % */
    {0x36,0x49,0x55,0x22,0x50}, /* 38 & */
    {0x00,0x05,0x03,0x00,0x00}, /* 39 ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40 ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41 ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* 42 * */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43 + */
    {0x00,0x50,0x30,0x00,0x00}, /* 44 , */
    {0x08,0x08,0x08,0x08,0x08}, /* 45 - */
    {0x00,0x60,0x60,0x00,0x00}, /* 46 . */
    {0x20,0x10,0x08,0x04,0x02}, /* 47 / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 50 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 53 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 55 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 56 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* 58 : */
    {0x00,0x56,0x36,0x00,0x00}, /* 59 ; */
    {0x00,0x08,0x14,0x22,0x41}, /* 60 < */
    {0x14,0x14,0x14,0x14,0x14}, /* 61 = */
    {0x41,0x22,0x14,0x08,0x00}, /* 62 > */
    {0x02,0x01,0x51,0x09,0x06}, /* 63 ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64 @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65 A */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66 B */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67 C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68 D */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69 E */
    {0x7F,0x09,0x09,0x01,0x01}, /* 70 F */
    {0x3E,0x41,0x41,0x51,0x32}, /* 71 G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72 H */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73 I */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74 J */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75 K */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76 L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 77 M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78 N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79 O */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80 P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81 Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82 R */
    {0x46,0x49,0x49,0x49,0x31}, /* 83 S */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84 T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85 U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86 V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* 87 W */
    {0x63,0x14,0x08,0x14,0x63}, /* 88 X */
    {0x03,0x04,0x78,0x04,0x03}, /* 89 Y */
    {0x61,0x51,0x49,0x45,0x43}, /* 90 Z */
};

/* ───────────────── OLED framebuffer and drawing ───────────────── */

/* SSD1306 page-based framebuffer: 128 cols x 4 pages (32 rows / 8) */
static uint8_t oled_fb[OLED_W * (OLED_H / 8)];
static esp_lcd_panel_handle_t oled_panel;

static void oled_clear(void)
{
    memset(oled_fb, 0, sizeof(oled_fb));
}

static void oled_set_pixel(int x, int y)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    oled_fb[x + (y / 8) * OLED_W] |= (1 << (y & 7));
}

static void oled_draw_char(int x, int y, char c)
{
    if (c < 32 || c > 90) return; /* only support space..Z */
    const uint8_t *glyph = font5x7[c - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                oled_set_pixel(x + col, y + row);
            }
        }
    }
}

static void oled_draw_str(int x, int y, const char *s)
{
    while (*s) {
        oled_draw_char(x, y, *s);
        x += 6; /* 5px char + 1px spacing */
        s++;
    }
}

/* Draw a horizontal hue position indicator bar */
static void oled_draw_hue_bar(int y, int height, uint16_t hue)
{
    /* Draw tick marks across the bar to suggest the rainbow spectrum */
    for (int x = 0; x < OLED_W; x++) {
        /* Bottom border line */
        oled_set_pixel(x, y + height - 1);
        /* Top border line */
        oled_set_pixel(x, y);
        /* Tick marks every 21 pixels (~60 degrees of hue) */
        if ((x % 21) == 0) {
            for (int r = 0; r < height; r++) {
                oled_set_pixel(x, y + r);
            }
        }
    }

    /* Draw filled marker at current hue position */
    int marker_x = (hue * (OLED_W - 1)) / 359;
    for (int dx = -2; dx <= 2; dx++) {
        for (int r = 0; r < height; r++) {
            oled_set_pixel(marker_x + dx, y + r);
        }
    }
}

static void oled_flush(void)
{
    esp_lcd_panel_draw_bitmap(oled_panel, 0, 0, OLED_W, OLED_H, oled_fb);
}

static void oled_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C bus for OLED (SDA=%d, SCL=%d)", OLED_I2C_SDA, OLED_I2C_SCL);

    /* I2C master bus */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_I2C_SDA,
        .scl_io_num = OLED_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    /* LCD panel IO over I2C */
    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = OLED_I2C_ADDR,
        .scl_speed_hz = 400000,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io));

    /* SSD1306 panel — must specify height=32 for 128x32 displays */
    esp_lcd_panel_ssd1306_config_t ssd1306_cfg = {
        .height = OLED_H,
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = &ssd1306_cfg,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_cfg, &oled_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(oled_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(oled_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(oled_panel, true));

    oled_clear();
    oled_flush();
    ESP_LOGI(TAG, "OLED initialized (128x32 SSD1306 @ 0x%02X)", OLED_I2C_ADDR);
}

/* ───────────────── LED strip (rainbow) ───────────────── */

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

/**
 * Convert HSV to RGB.
 * h: 0-359, s: 0-255, v: 0-255
 */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h = h % 360;
    uint8_t region = h / 60;
    uint8_t remainder = (h % 60) * 255 / 60;

    uint8_t p = (v * (255 - s)) / 255;
    uint8_t q = (v * (255 - (s * remainder) / 255)) / 255;
    uint8_t t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

static void blink_led(void)
{
    uint8_t r, g, b;
    /* Convert current hue to RGB (full saturation, low brightness) */
    hsv_to_rgb(s_hue, 255, 32, &r, &g, &b);
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
    /* Advance hue by 10 degrees each cycle (full rainbow every 36 steps) */
    s_hue = (s_hue + 10) % 360;
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

/* ───────────────── Main ───────────────── */

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

    /* Initialize the OLED display */
    oled_init();

    char line_buf[22]; /* 21 chars max per line + null */

    while (1) {
        /* Get the RGB values that will be set this cycle */
        uint8_t r, g, b;
        hsv_to_rgb(s_hue, 255, 32, &r, &g, &b);

        ESP_LOGI(TAG, "Hue: %d  R:%d G:%d B:%d", s_hue, r, g, b);

        /* Update LED */
        blink_led();

        /* Update OLED display */
        oled_clear();

        snprintf(line_buf, sizeof(line_buf), "HUE: %3d", s_hue);
        oled_draw_str(0, 0, line_buf);

        snprintf(line_buf, sizeof(line_buf), "R:%3d G:%3d B:%3d", r, g, b);
        oled_draw_str(0, 10, line_buf);

        /* Hue position bar at bottom */
        oled_draw_hue_bar(22, 9, s_hue);

        oled_flush();

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
