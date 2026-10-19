/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#include "screen_status.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void draw_base_frame(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
    mono_clear(cbuf, COLOR_BLACK);

    // 4 Corner validation marks
    mono_draw_pixel(cbuf, 0, 0, COLOR_WHITE);     // Top-Left (0, 0)
    mono_draw_pixel(cbuf, 31, 0, COLOR_WHITE);    // Top-Right (31, 0)
    mono_draw_pixel(cbuf, 0, 127, COLOR_WHITE);   // Bottom-Left (0, 127)
    mono_draw_pixel(cbuf, 31, 127, COLOR_WHITE);  // Bottom-Right (31, 127)

    // Structural Dividers
    mono_draw_line_h(cbuf, 0, 31, 24, COLOR_WHITE); // Header divider
    mono_draw_line_h(cbuf, 0, 31, 88, COLOR_WHITE); // Footer divider

    // Text labels
    mono_draw_string(cbuf, 3, 4, "CORNE", COLOR_WHITE);
    mono_draw_string(cbuf, 3, 50, "READY", COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_obj_invalidate(canvas);
    }
}

int zmk_widget_screen_status_init(struct zmk_widget_screen_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    draw_base_frame(widget);
    return 0;
}

lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget) {
    return widget->obj;
}
