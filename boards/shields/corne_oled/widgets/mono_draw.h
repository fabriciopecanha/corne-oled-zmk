/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>

#define CORNE_LOGICAL_WIDTH   32
#define CORNE_LOGICAL_HEIGHT 128

#define CORNE_CANVAS_WIDTH   128
#define CORNE_CANVAS_HEIGHT   32

#define CORNE_CANVAS_PIXELS  (CORNE_CANVAS_WIDTH * CORNE_CANVAS_HEIGHT)

/*
 * Polarity configuration:
 * The SSD1306 display is initialized with 'inversion-on;' in Devicetree,
 * which sets REVERSE_DISPLAY (0xA7). In this mode:
 * - A bit value of 1 turns the physical pixel OFF (pure black OLED background).
 * - A bit value of 0 turns the physical pixel ON (crisp white text/lines).
 */
#define COLOR_BLACK ((lv_color_t){.full = 1})
#define COLOR_WHITE ((lv_color_t){.full = 0})

void mono_clear(lv_color_t *cbuf, lv_color_t color);
void mono_draw_pixel(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_line_h(lv_color_t *cbuf, int x0, int x1, int y, lv_color_t color);
void mono_draw_line_v(lv_color_t *cbuf, int x, int y0, int y1, lv_color_t color);
void mono_draw_rect(lv_color_t *cbuf, int x, int y, int w, int h, lv_color_t color);
void mono_fill_rect(lv_color_t *cbuf, int x, int y, int w, int h, lv_color_t color);
void mono_draw_char(lv_color_t *cbuf, int x, int y, char ch, lv_color_t color);
void mono_draw_string(lv_color_t *cbuf, int x, int y, const char *str, lv_color_t color);
void mono_draw_big_digit(lv_color_t *cbuf, int x, int y, int digit, lv_color_t color);
void mono_draw_hero_digit(lv_color_t *cbuf, int x, int y, int digit, lv_color_t color);
void mono_draw_battery(lv_color_t *cbuf, int x, int y, uint8_t level, bool charging);
void mono_draw_battery_large(lv_color_t *cbuf, int x, int y, uint8_t level, bool charging);
void mono_draw_bt_icon(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_apple_shift(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_apple_ctrl(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_apple_opt(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_apple_cmd(lv_color_t *cbuf, int x, int y, lv_color_t color);
void mono_draw_bold_corne(lv_color_t *cbuf, int x, int y_start, lv_color_t color);

