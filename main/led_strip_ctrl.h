#pragma once

#include <stdint.h>

/** Initialize and configure the LED strip. */
void led_init(void);

/** Advance the hue and update the LED color. */
void led_blink(void);

/** Get the current hue value (0-359). */
uint16_t led_get_hue(void);

/**
 * Convert HSV to RGB.
 * h: 0-359, s: 0-255, v: 0-255
 */
void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
