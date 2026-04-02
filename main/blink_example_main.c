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
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_app_desc.h"
#include "ota_update.h"
#include "oled_display.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint16_t s_hue = 0;

/* OTA button */
#define OTA_BUTTON_GPIO CONFIG_OTA_BUTTON_GPIO

/* OTA display state (written by callback from OTA task, read by main loop) */
static volatile int s_ota_percent = -1;
static char s_ota_status[16] = "";

static void ota_progress_callback(int percent, const char *status_msg)
{
    s_ota_percent = percent;
    if (status_msg) {
        strncpy(s_ota_status, status_msg, sizeof(s_ota_status) - 1);
        s_ota_status[sizeof(s_ota_status) - 1] = '\0';
    }
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

    /* Configure OTA button (active-low with pull-up) */
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << OTA_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    /* Register OTA progress callback */
    ota_set_progress_callback(ota_progress_callback);

    char line_buf[22]; /* 21 chars max per line + null */
    int btn_debounce = 0;

    while (1) {
        /* Check OTA button (active-low) */
        if (gpio_get_level(OTA_BUTTON_GPIO) == 0) {
            btn_debounce++;
            if (btn_debounce == 3 && !ota_is_in_progress()) {
                ESP_LOGI(TAG, "OTA button pressed, starting update...");
                ota_start_update();
            }
        } else {
            btn_debounce = 0;
        }

        /* Update LED (keeps running during OTA as alive indicator) */
        blink_led();

        /* Update OLED display */
        oled_clear();

        if (ota_is_in_progress()) {
            /* OTA progress display */
            oled_draw_str(0, 0, "OTA UPDATE");
            oled_draw_str(0, 10, s_ota_status);
            int pct = s_ota_percent;
            if (pct >= 0) {
                oled_draw_progress_bar(22, 9, pct);
            }
        } else {
            /* Normal display with version */
            const esp_app_desc_t *app = esp_app_get_description();
            uint8_t r, g, b;
            hsv_to_rgb(s_hue, 255, 32, &r, &g, &b);

            ESP_LOGI(TAG, "Hue: %d  R:%d G:%d B:%d", s_hue, r, g, b);

            snprintf(line_buf, sizeof(line_buf), "V%.5s H:%3d", app->version, s_hue);
            oled_draw_str(0, 0, line_buf);

            snprintf(line_buf, sizeof(line_buf), "R:%3d G:%3d B:%3d", r, g, b);
            oled_draw_str(0, 10, line_buf);

            /* Hue position bar at bottom */
            oled_draw_hue_bar(22, 9, s_hue);
        }

        oled_flush();

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}

