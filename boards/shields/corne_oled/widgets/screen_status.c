/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#include "screen_status.h"
#include <stdio.h>
#include <string.h>
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
#include <zmk/events/layer_state_changed.h>
#include <zmk/ble.h>
#include <zmk/keymap.h>
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

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};
#else
struct peripheral_status_state {
    bool connected;
};
#endif

// -----------------------------------------------------------------------------
// Regional Rendering & True Partial Updates
// -----------------------------------------------------------------------------
// Header Region (Logical Y: 0..24 -> Canvas X: 103..127)
static void draw_header_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
    mono_fill_rect(cbuf, 0, 0, 32, 25, COLOR_BLACK);

    // Top Corners
    mono_draw_pixel(cbuf, 0, 0, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 0, COLOR_WHITE);

    // Header Divider (Y=24)
    mono_draw_line_h(cbuf, 0, 31, 24, COLOR_WHITE);

    // Battery (Y=4..10)
    char batt_str[6] = {};
    snprintf(batt_str, sizeof(batt_str), "%u%%", widget->state.battery);
    mono_draw_string(cbuf, 2, 4, batt_str, COLOR_WHITE);
    mono_draw_battery(cbuf, 19, 4, widget->state.battery, widget->state.charging);

    // Connection / Role (Y=14..20)
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
    // Peripheral: "RIGHT" centered with 4px margins (24px text width in 32px)
    mono_draw_string(cbuf, 4, 14, "RIGHT", COLOR_WHITE);
#endif

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 103, .y1 = 0, .x2 = 127, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
}

// Body Region (Logical Y: 25..87 -> Canvas X: 40..102)
static void draw_body_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
    mono_fill_rect(cbuf, 0, 25, 32, 63, COLOR_BLACK);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    // Big Layer Digit (Y=36..58, centered)
    mono_draw_big_digit(cbuf, 9, 36, widget->state.layer_index, COLOR_WHITE);

    // Layer Name string (Y=68..74, safely truncated to 6 characters max)
    const char *name = widget->state.layer_label;
    char safe_name[7] = {};
    if (!name || strlen(name) == 0) {
        snprintf(safe_name, sizeof(safe_name), "L%u", widget->state.layer_index);
    } else {
        strncpy(safe_name, name, 6);
        safe_name[6] = '\0';
    }
    int text_len = strlen(safe_name);
    int text_w = text_len * 5 - 1;
    int text_x = (CORNE_LOGICAL_WIDTH - text_w) / 2;
    if (text_x < 1) text_x = 1;
    mono_draw_string(cbuf, text_x, 68, safe_name, COLOR_WHITE);
#else
    // Peripheral: Vertical CORNE box (Y=34..79)
    mono_draw_rect(cbuf, 5, 34, 22, 45, COLOR_WHITE);
    mono_draw_char(cbuf, 14, 40, 'C', COLOR_WHITE);
    mono_draw_char(cbuf, 14, 48, 'O', COLOR_WHITE);
    mono_draw_char(cbuf, 14, 56, 'R', COLOR_WHITE);
    mono_draw_char(cbuf, 14, 64, 'N', COLOR_WHITE);
    mono_draw_char(cbuf, 14, 72, 'E', COLOR_WHITE);
#endif

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 40, .y1 = 0, .x2 = 102, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
}

// Footer Region (Logical Y: 88..127 -> Canvas X: 0..39)
static void draw_footer_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
    mono_fill_rect(cbuf, 0, 88, 32, 40, COLOR_BLACK);

    // Footer Divider (Y=88)
    mono_draw_line_h(cbuf, 0, 31, 88, COLOR_WHITE);

    // Bottom Corners
    mono_draw_pixel(cbuf, 0, 127, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 127, COLOR_WHITE);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    // Central footer: reserved for future modifier indicator blocks
#else
    // Peripheral footer: SPLIT (Y=96) + OK/-- (Y=108)
    mono_draw_string(cbuf, 4, 96, "SPLIT", COLOR_WHITE);
    mono_draw_string(cbuf, 11, 108, widget->state.connected ? "OK" : "--", COLOR_WHITE);
#endif

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 0, .y1 = 0, .x2 = 39, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
}

static void draw_full_screen(struct zmk_widget_screen_status *widget) {
    draw_header_region(widget);
    draw_body_region(widget);
    draw_footer_region(widget);
}

// -----------------------------------------------------------------------------
// Battery Listener
// -----------------------------------------------------------------------------
static void set_battery_status(struct zmk_widget_screen_status *widget,
                               struct battery_status_state state) {
    if (widget->state.battery == state.level && widget->state.charging == state.usb_present) {
        return;
    }
    widget->state.battery = state.level;
    widget->state.charging = state.usb_present;
    draw_header_region(widget);
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
    if (widget->state.selected_endpoint.transport == state.selected_endpoint.transport &&
        widget->state.selected_endpoint.ble.profile_index == state.selected_endpoint.ble.profile_index &&
        widget->state.active_profile_connected == state.active_profile_connected &&
        widget->state.active_profile_bonded == state.active_profile_bonded) {
        return;
    }
    widget->state.selected_endpoint = state.selected_endpoint;
    widget->state.active_profile_connected = state.active_profile_connected;
    widget->state.active_profile_bonded = state.active_profile_bonded;
    draw_header_region(widget);
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

// -----------------------------------------------------------------------------
// Layer Status Listener (Central only)
// -----------------------------------------------------------------------------
static void set_layer_status(struct zmk_widget_screen_status *widget,
                             struct layer_status_state state) {
    if (widget->state.layer_index == state.index &&
        widget->state.layer_label == state.label) {
        return;
    }
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;
    draw_body_region(widget);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_screen_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_layer_status(widget, state);
    }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
        .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index))
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state);

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

#else // Peripheral split role
static void set_peripheral_status(struct zmk_widget_screen_status *widget,
                                  struct peripheral_status_state state) {
    if (widget->state.connected == state.connected) {
        return;
    }
    widget->state.connected = state.connected;
    draw_footer_region(widget);
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
    widget_layer_status_init();
#else
    widget_peripheral_status_init();
#endif

    draw_full_screen(widget);
    return 0;
}

lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget) {
    return widget->obj;
}
