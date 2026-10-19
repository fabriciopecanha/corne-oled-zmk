/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "mono_draw.h"

struct screen_status_state {
    uint8_t battery;
    bool charging;
};

struct zmk_widget_screen_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[CORNE_CANVAS_PIXELS];
    struct screen_status_state state;
};

int zmk_widget_screen_status_init(struct zmk_widget_screen_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget);
