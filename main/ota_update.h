#pragma once

#include <stdbool.h>

/**
 * Progress callback invoked during OTA update.
 * @param percent  Download progress 0-100 (or -1 for status-only messages)
 * @param status_msg  Short status string for display (e.g. "WIFI...", "48%", "DONE")
 */
typedef void (*ota_progress_cb_t)(int percent, const char *status_msg);

/** Register a callback to receive OTA progress updates (for OLED display). */
void ota_set_progress_callback(ota_progress_cb_t cb);

/** Launch an OTA update in a background FreeRTOS task. */
void ota_start_update(void);

/** Returns true if an OTA update is currently in progress. */
bool ota_is_in_progress(void);
