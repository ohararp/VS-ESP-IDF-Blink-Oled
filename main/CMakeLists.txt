idf_component_register(SRCS "oled_display.c" "blink_example_main.c" "ota_update.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES nvs_flash esp_wifi esp_netif esp_https_ota esp_http_client app_update esp_driver_gpio esp_driver_i2c esp_lcd esp_app_format mbedtls)
