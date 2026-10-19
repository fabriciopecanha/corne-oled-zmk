/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/endpoints.h>
#include "mono_draw.h"

struct screen_status_state {
    uint8_t battery;
    bool charging;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
#else
    bool connected;
#endif
};

struct zmk_widget_screen_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[CORNE_CANVAS_PIXELS];
    struct screen_status_state state;
};

int zmk_widget_screen_status_init(struct zmk_widget_screen_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget);
