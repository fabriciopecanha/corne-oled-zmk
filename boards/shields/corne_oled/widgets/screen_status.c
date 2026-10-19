/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#include "screen_status.h"
#include <stdio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_status_state {
    uint8_t level;
    bool usb_present;
};

static void draw_screen(struct zmk_widget_screen_status *widget) {
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

    // Battery display (Top Header: Y=4..10)
    char batt_str[6] = {};
    snprintf(batt_str, sizeof(batt_str), "%u%%", widget->state.battery);
    mono_draw_string(cbuf, 2, 4, batt_str, COLOR_WHITE);
    mono_draw_battery(cbuf, 19, 4, widget->state.battery, widget->state.charging);

    // Baseline text
    mono_draw_string(cbuf, 3, 50, "CORNE", COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_obj_invalidate(canvas);
    }
}

static void set_battery_status(struct zmk_widget_screen_status *widget,
                               struct battery_status_state state) {
    widget->state.battery = state.level;
    widget->state.charging = state.usb_present;
    draw_screen(widget);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_screen_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_status(widget, state);
    }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#else
        .usb_present = false,
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state);

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif

int zmk_widget_screen_status_init(struct zmk_widget_screen_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();
    draw_screen(widget);
    return 0;
}

lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget) {
    return widget->obj;
}
