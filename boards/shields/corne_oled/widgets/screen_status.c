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

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/ble.h>
#else
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_status_state {
    uint8_t level;
    bool usb_present;
};

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};
#else
struct peripheral_status_state {
    bool connected;
};
#endif

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

    // Battery display (Top Header Row 1: Y=4..10)
    char batt_str[6] = {};
    snprintf(batt_str, sizeof(batt_str), "%u%%", widget->state.battery);
    mono_draw_string(cbuf, 2, 4, batt_str, COLOR_WHITE);
    mono_draw_battery(cbuf, 19, 4, widget->state.battery, widget->state.charging);

    // Output Connection display (Top Header Row 2: Y=14..20)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (widget->state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        mono_draw_string(cbuf, 2, 14, "USB", COLOR_WHITE);
        mono_draw_string(cbuf, 20, 14, "OK", COLOR_WHITE);
        break;
    case ZMK_TRANSPORT_BLE: {
        char bt_str[6] = {};
        snprintf(bt_str, sizeof(bt_str), "BT%d", widget->state.selected_endpoint.ble.profile_index + 1);
        mono_draw_string(cbuf, 2, 14, bt_str, COLOR_WHITE);
        if (widget->state.active_profile_connected) {
            mono_draw_string(cbuf, 20, 14, "OK", COLOR_WHITE);
        } else if (widget->state.active_profile_bonded) {
            mono_draw_string(cbuf, 20, 14, "--", COLOR_WHITE);
        } else {
            mono_draw_string(cbuf, 20, 14, "?", COLOR_WHITE);
        }
        break;
    }
    }
#else
    // Peripheral screen
    mono_draw_string(cbuf, 2, 14, "SPLIT", COLOR_WHITE);
    mono_draw_string(cbuf, 26, 14, "R", COLOR_WHITE);

    // Footer on peripheral
    mono_draw_string(cbuf, 6, 96, "SYNC", COLOR_WHITE);
    mono_draw_string(cbuf, 11, 108, widget->state.connected ? "OK" : "--", COLOR_WHITE);
#endif

    // Baseline text
    mono_draw_string(cbuf, 3, 50, "CORNE", COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_obj_invalidate(canvas);
    }
}

// -----------------------------------------------------------------------------
// Battery Listener
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Output Status Listener (Central vs Peripheral)
// -----------------------------------------------------------------------------
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static void set_output_status(struct zmk_widget_screen_status *widget,
                              struct output_status_state state) {
    widget->state.selected_endpoint = state.selected_endpoint;
    widget->state.active_profile_connected = state.active_profile_connected;
    widget->state.active_profile_bonded = state.active_profile_bonded;
    draw_screen(widget);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_screen_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_output_status(widget, state);
    }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open()
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state);

ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

#else // Peripheral split role
static void set_peripheral_status(struct zmk_widget_screen_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;
    draw_screen(widget);
}

static void peripheral_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_screen_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_peripheral_status(widget, state);
    }
}

static struct peripheral_status_state peripheral_status_get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){
        .connected = zmk_split_bt_peripheral_is_connected()
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            peripheral_status_update_cb, peripheral_status_get_state);

ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);
#endif

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------
int zmk_widget_screen_status_init(struct zmk_widget_screen_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    widget_output_status_init();
#else
    widget_peripheral_status_init();
#endif

    draw_screen(widget);
    return 0;
}

lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget) {
    return widget->obj;
}
