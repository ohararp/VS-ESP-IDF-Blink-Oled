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
#include "sdkconfig.h"
#include "esp_app_desc.h"
#include "ota_update.h"
#include "oled_display.h"
#include "led_strip_ctrl.h"

static const char *TAG = "example";

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

/* ───────────────── Main ───────────────── */

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    led_init();

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
        led_blink();

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
            uint16_t hue = led_get_hue();
            uint8_t r, g, b;
            hsv_to_rgb(hue, 255, 32, &r, &g, &b);

            ESP_LOGI(TAG, "Hue: %d  R:%d G:%d B:%d", hue, r, g, b);

            snprintf(line_buf, sizeof(line_buf), "V%.5s H:%3d", app->version, hue);
            oled_draw_str(0, 0, line_buf);

            snprintf(line_buf, sizeof(line_buf), "R:%3d G:%3d B:%3d", r, g, b);
            oled_draw_str(0, 10, line_buf);

            /* Hue position bar at bottom */
            oled_draw_hue_bar(22, 9, hue);
        }

        oled_flush();

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
