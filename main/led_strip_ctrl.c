#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "led_strip_ctrl.h"

static const char *TAG = "led";

#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint16_t s_hue = 0;

void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
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

uint16_t led_get_hue(void)
{
    return s_hue;
}

/* ───────────────── LED strip (rainbow) ───────────────── */

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

void led_blink(void)
{
    uint8_t r, g, b;
    /* Convert current hue to RGB (full saturation, low brightness) */
    hsv_to_rgb(s_hue, 255, 32, &r, &g, &b);
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
    /* Advance hue by 10 degrees each cycle (full rainbow every 36 steps) */
    s_hue = (s_hue + 10) % 360;
}

void led_init(void)
{
    ESP_LOGI(TAG, "Configured to blink addressable LED on GPIO %d", BLINK_GPIO);
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
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
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

void led_blink(void)
{
    gpio_set_level(BLINK_GPIO, s_led_state);
}

void led_init(void)
{
    ESP_LOGI(TAG, "Configured to blink GPIO LED on GPIO %d", BLINK_GPIO);
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif
