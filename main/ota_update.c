#include "ota_update.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "ota";

static ota_progress_cb_t s_progress_cb = NULL;
static volatile bool s_ota_in_progress = false;

/* Event group bits for WiFi connection */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;

void ota_set_progress_callback(ota_progress_cb_t cb)
{
    s_progress_cb = cb;
}

bool ota_is_in_progress(void)
{
    return s_ota_in_progress;
}

static void report_progress(int percent, const char *msg)
{
    if (s_progress_cb) {
        s_progress_cb(percent, msg);
    }
}

/* ───────────────── WiFi event handler ───────────────── */

static int s_retry_count = 0;
#define MAX_RETRY 5

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)", s_retry_count, MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ───────────────── WiFi connect / disconnect ───────────────── */

static bool wifi_connect(void)
{
    s_retry_count = 0;
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst_any_id;
    esp_event_handler_instance_t inst_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, &inst_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, &inst_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_OTA_WIFI_SSID,
            .password = CONFIG_OTA_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    report_progress(-1, "WIFI...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                       pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, inst_any_id);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, inst_got_ip);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        report_progress(-1, "WIFI OK");
        return true;
    }

    ESP_LOGE(TAG, "WiFi connection failed");
    report_progress(-1, "WIFI FAIL");
    return false;
}

static void wifi_disconnect(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
    vEventGroupDelete(s_wifi_event_group);
}

/* ───────────────── OTA download ───────────────── */

static void ota_task(void *pvParameter)
{
    s_ota_in_progress = true;

    /* Initialize NVS (required for WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    report_progress(-1, "OTA UPDATE");

    if (!wifi_connect()) {
        report_progress(-1, "WIFI FAIL");
        vTaskDelay(pdMS_TO_TICKS(5000));
        wifi_disconnect();
        s_ota_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    esp_http_client_config_t http_config = {
        .url = CONFIG_OTA_FIRMWARE_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 10240,
        .buffer_size_tx = 10240,
        .max_redirection_count = 5,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    ret = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(ret));
        report_progress(-1, "OTA FAIL");
        vTaskDelay(pdMS_TO_TICKS(5000));
        wifi_disconnect();
        s_ota_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    int total_size = esp_https_ota_get_image_size(https_ota_handle);
    ESP_LOGI(TAG, "OTA image size: %d bytes", total_size);

    char status_buf[16];
    while (1) {
        ret = esp_https_ota_perform(https_ota_handle);
        if (ret != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        int read_size = esp_https_ota_get_image_len_read(https_ota_handle);
        if (total_size > 0) {
            int percent = (read_size * 100) / total_size;
            snprintf(status_buf, sizeof(status_buf), "%d%%", percent);
            report_progress(percent, status_buf);
        }
    }

    if (ret == ESP_OK) {
        ret = esp_https_ota_finish(https_ota_handle);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "OTA update successful, rebooting...");
            report_progress(100, "REBOOTING");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        }
    }

    /* If we get here, OTA failed */
    ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    esp_https_ota_abort(https_ota_handle);
    report_progress(-1, "OTA FAIL");
    vTaskDelay(pdMS_TO_TICKS(5000));
    wifi_disconnect();
    s_ota_in_progress = false;
    vTaskDelete(NULL);
}

void ota_start_update(void)
{
    if (s_ota_in_progress) {
        ESP_LOGW(TAG, "OTA already in progress");
        return;
    }
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
}
