#pragma once

#include "esp_lcd_panel_ops.h"

#define OLED_W 128
#define OLED_H 32

/** Initialize the SSD1306 OLED display over I2C. */
void oled_init(void);

/** Clear the framebuffer (all pixels off). */
void oled_clear(void);

/** Set a single pixel in the framebuffer. */
void oled_set_pixel(int x, int y);

/** Draw a single character at (x, y) using the built-in 5x7 font. */
void oled_draw_char(int x, int y, char c);

/** Draw a null-terminated string at (x, y). */
void oled_draw_str(int x, int y, const char *s);

/** Draw a horizontal hue position indicator bar. */
void oled_draw_hue_bar(int y, int height, uint16_t hue);

/** Draw a progress bar (0-100%). */
void oled_draw_progress_bar(int y, int height, int percent);

/** Flush the framebuffer to the display. */
void oled_flush(void);
