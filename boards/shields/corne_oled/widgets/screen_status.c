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
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/keys.h>
#include <zmk/ble.h>
#include <zmk/keymap.h>

#ifndef HID_USAGE_KEY_KEYBOARD_LEFTCONTROL
#define HID_USAGE_KEY_KEYBOARD_LEFTCONTROL 0xE0
#endif

#ifndef MOD_LCTL
#define MOD_LCTL 0x01
#endif
#ifndef MOD_LSFT
#define MOD_LSFT 0x02
#endif
#ifndef MOD_LALT
#define MOD_LALT 0x04
#endif
#ifndef MOD_LGUI
#define MOD_LGUI 0x08
#endif
#ifndef MOD_RCTL
#define MOD_RCTL 0x10
#endif
#ifndef MOD_RSFT
#define MOD_RSFT 0x20
#endif
#ifndef MOD_RALT
#define MOD_RALT 0x40
#endif
#ifndef MOD_RGUI
#define MOD_RGUI 0x80
#endif
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

struct mods_status_state {
    uint8_t mods;
};
#else
struct peripheral_status_state {
    bool connected;
};
#endif

// -----------------------------------------------------------------------------
// Regional Rendering & True Partial Updates (Variant V2 Tech Brackets + M1 Left)
// -----------------------------------------------------------------------------
// Header Region (Central: Y: 0..25 -> Canvas X: 102..127, 26 cols)
//               (Peripheral: Y: 0..23 -> Canvas X: 104..127, 24 cols)
static void draw_header_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    mono_fill_rect(cbuf, 0, 0, 32, 26, COLOR_BLACK);

    // Top Corners
    mono_draw_pixel(cbuf, 0, 0, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 0, COLOR_WHITE);

    // Header Divider (Y=25)
    mono_draw_line_h(cbuf, 0, 31, 25, COLOR_WHITE);

    // Large Battery (x=1, y=2)
    mono_draw_battery_large(cbuf, 1, 2, widget->state.battery, widget->state.charging);
    char batt_str[6] = {};
    if (widget->state.battery >= 100) {
        mono_draw_string(cbuf, 16, 3, "100", COLOR_WHITE);
    } else {
        snprintf(batt_str, sizeof(batt_str), "%u%%", widget->state.battery);
        mono_draw_string(cbuf, 16, 3, batt_str, COLOR_WHITE);
    }

    // Connection: Bluetooth or USB (Y=14..20)
    switch (widget->state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        mono_draw_string(cbuf, 2, 15, "USB", COLOR_WHITE);
        mono_draw_string(cbuf, 20, 15, "OK", COLOR_WHITE);
        break;
    case ZMK_TRANSPORT_BLE: {
        mono_draw_bt_icon(cbuf, 2, 14, COLOR_WHITE);
        char bt_str[6] = {};
        snprintf(bt_str, sizeof(bt_str), "B%d", widget->state.selected_endpoint.ble.profile_index + 1);
        mono_draw_string(cbuf, 10, 15, bt_str, COLOR_WHITE);
        if (widget->state.active_profile_connected) {
            mono_draw_string(cbuf, 22, 15, "OK", COLOR_WHITE);
        } else if (widget->state.active_profile_bonded) {
            mono_draw_string(cbuf, 22, 15, "--", COLOR_WHITE);
        } else {
            mono_draw_string(cbuf, 22, 15, "?", COLOR_WHITE);
        }
        break;
    }
    }

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 102, .y1 = 0, .x2 = 127, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#else
    mono_fill_rect(cbuf, 0, 0, 32, 24, COLOR_BLACK);

    // Top Corners
    mono_draw_pixel(cbuf, 0, 0, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 0, COLOR_WHITE);

    // Header Divider (Y=23)
    mono_draw_line_h(cbuf, 0, 31, 23, COLOR_WHITE);

    // Battery (Y=4..10)
    char batt_str[6] = {};
    snprintf(batt_str, sizeof(batt_str), "%u%%", widget->state.battery);
    mono_draw_string(cbuf, 2, 4, batt_str, COLOR_WHITE);
    mono_draw_battery(cbuf, 19, 4, widget->state.battery, widget->state.charging);

    // Peripheral: "RIGHT" centered with 4px margins (24px text width in 32px)
    mono_draw_string(cbuf, 4, 14, "RIGHT", COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 104, .y1 = 0, .x2 = 127, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#endif
}

// Body Region (Central: Y: 26..88 -> Canvas X: 39..101, 63 cols)
//             (Peripheral: Y: 24..89 -> Canvas X: 38..103, 66 cols)
static void draw_body_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    mono_fill_rect(cbuf, 0, 26, 32, 63, COLOR_BLACK);

    // Brackets (Y=28..87)
    mono_draw_line_v(cbuf, 4, 28, 87, COLOR_WHITE);
    mono_draw_line_h(cbuf, 4, 8, 28, COLOR_WHITE);
    mono_draw_line_h(cbuf, 4, 8, 87, COLOR_WHITE);

    mono_draw_line_v(cbuf, 27, 28, 87, COLOR_WHITE);
    mono_draw_line_h(cbuf, 23, 27, 28, COLOR_WHITE);
    mono_draw_line_h(cbuf, 23, 27, 87, COLOR_WHITE);

    // Hero Digit (Y=33..66)
    mono_draw_hero_digit(cbuf, 6, 33, widget->state.layer_index, COLOR_WHITE);

    // Divider (Y=71)
    mono_draw_line_h(cbuf, 10, 21, 71, COLOR_WHITE);

    // Layer Name string (Y=76, safely truncated to 6 characters max)
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
    mono_draw_string(cbuf, text_x, 76, safe_name, COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 39, .y1 = 0, .x2 = 101, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#else
    mono_fill_rect(cbuf, 0, 24, 32, 66, COLOR_BLACK);

    // Peripheral: Tech brackets + Bold CORNE (Variant V2)
    // Left bracket (X=4, Y=26..87)
    mono_draw_line_v(cbuf, 4, 26, 87, COLOR_WHITE);
    mono_draw_line_h(cbuf, 4, 8, 26, COLOR_WHITE);
    mono_draw_line_h(cbuf, 4, 8, 87, COLOR_WHITE);

    // Right bracket (X=27, Y=26..87)
    mono_draw_line_v(cbuf, 27, 26, 87, COLOR_WHITE);
    mono_draw_line_h(cbuf, 23, 27, 26, COLOR_WHITE);
    mono_draw_line_h(cbuf, 23, 27, 87, COLOR_WHITE);

    // Bold CORNE (centered at X=11, starting at Y=28)
    mono_draw_bold_corne(cbuf, 11, 28, COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 38, .y1 = 0, .x2 = 103, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#endif
}

// Footer Region (Central: Y: 89..127 -> Canvas X: 0..38, 39 cols)
//             (Peripheral: Y: 90..127 -> Canvas X: 0..37, 38 cols)
static void draw_footer_region(struct zmk_widget_screen_status *widget) {
    lv_color_t *cbuf = widget->cbuf;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    mono_fill_rect(cbuf, 0, 89, 32, 39, COLOR_BLACK);

    // Footer Divider (Y=89)
    mono_draw_line_h(cbuf, 0, 31, 89, COLOR_WHITE);

    // Bottom Corners
    mono_draw_pixel(cbuf, 0, 127, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 127, COLOR_WHITE);

    uint8_t mods = widget->state.modifiers;

    // Box 1: Shift [⇧] (X=2..14, Y=92..106)
    bool s_act = (mods & (MOD_LSFT | MOD_RSFT)) != 0;
    if (s_act) {
        mono_fill_rect(cbuf, 2, 92, 13, 15, COLOR_WHITE);
        mono_draw_apple_shift(cbuf, 4, 95, COLOR_BLACK);
    } else {
        mono_draw_rect(cbuf, 2, 92, 13, 15, COLOR_WHITE);
        mono_draw_apple_shift(cbuf, 4, 95, COLOR_WHITE);
    }

    // Box 2: Ctrl [⌃] (X=17..29, Y=92..106)
    bool c_act = (mods & (MOD_LCTL | MOD_RCTL)) != 0;
    if (c_act) {
        mono_fill_rect(cbuf, 17, 92, 13, 15, COLOR_WHITE);
        mono_draw_apple_ctrl(cbuf, 19, 97, COLOR_BLACK);
    } else {
        mono_draw_rect(cbuf, 17, 92, 13, 15, COLOR_WHITE);
        mono_draw_apple_ctrl(cbuf, 19, 97, COLOR_WHITE);
    }

    // Box 3: Opt [⌥] (X=2..14, Y=109..123)
    bool a_act = (mods & (MOD_LALT | MOD_RALT)) != 0;
    if (a_act) {
        mono_fill_rect(cbuf, 2, 109, 13, 15, COLOR_WHITE);
        mono_draw_apple_opt(cbuf, 4, 114, COLOR_BLACK);
    } else {
        mono_draw_rect(cbuf, 2, 109, 13, 15, COLOR_WHITE);
        mono_draw_apple_opt(cbuf, 4, 114, COLOR_WHITE);
    }

    // Box 4: Cmd [⌘] (X=17..29, Y=109..123)
    bool g_act = (mods & (MOD_LGUI | MOD_RGUI)) != 0;
    if (g_act) {
        mono_fill_rect(cbuf, 17, 109, 13, 15, COLOR_WHITE);
        mono_draw_apple_cmd(cbuf, 19, 112, COLOR_BLACK);
    } else {
        mono_draw_rect(cbuf, 17, 109, 13, 15, COLOR_WHITE);
        mono_draw_apple_cmd(cbuf, 19, 112, COLOR_WHITE);
    }

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 0, .y1 = 0, .x2 = 38, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#else
    mono_fill_rect(cbuf, 0, 90, 32, 38, COLOR_BLACK);

    // Footer Divider (Y=90)
    mono_draw_line_h(cbuf, 0, 31, 90, COLOR_WHITE);

    // Bottom Corners
    mono_draw_pixel(cbuf, 0, 127, COLOR_WHITE);
    mono_draw_pixel(cbuf, 31, 127, COLOR_WHITE);

    // Peripheral footer: SPLIT (Y=96) + OK/-- (Y=109)
    mono_draw_string(cbuf, 4, 96, "SPLIT", COLOR_WHITE);
    mono_draw_string(cbuf, 11, 109, widget->state.connected ? "OK" : "--", COLOR_WHITE);

    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    if (canvas) {
        lv_area_t area = { .x1 = 0, .y1 = 0, .x2 = 37, .y2 = 31 };
        lv_obj_invalidate_area(canvas, &area);
    }
#endif
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

// -----------------------------------------------------------------------------
// Modifier Status Listener (Central only)
// -----------------------------------------------------------------------------
static inline bool is_modifier_key(uint8_t usage_page, uint32_t keycode) {
    return (keycode >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL &&
            keycode <= (HID_USAGE_KEY_KEYBOARD_LEFTCONTROL + 7) &&
            usage_page == 0x07);
}

static void set_mods_status(struct zmk_widget_screen_status *widget,
                            struct mods_status_state state) {
    if (widget->state.modifiers == state.mods) {
        return;
    }
    widget->state.modifiers = state.mods;
    draw_footer_region(widget);
}

static void mods_status_update_cb(struct mods_status_state state) {
    uint8_t live_mods = (uint8_t)zmk_hid_get_explicit_mods();
    if (live_mods == 0) {
        state.mods = 0;
    }
    struct zmk_widget_screen_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_mods_status(widget, state);
    }
}

static struct mods_status_state mods_status_get_state(const zmk_event_t *eh) {
    uint8_t mods = (uint8_t)zmk_hid_get_explicit_mods();

    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev != NULL) {
        uint8_t mod_bit = ev->explicit_modifiers;
        if (is_modifier_key(ev->usage_page, ev->keycode)) {
            mod_bit |= (1 << (ev->keycode - HID_USAGE_KEY_KEYBOARD_LEFTCONTROL));
        }
        if (ev->state) {
            mods |= mod_bit;
        } else {
            mods &= ~mod_bit;
        }
        return (struct mods_status_state){ .mods = mods };
    }

    const struct zmk_modifiers_state_changed *m_ev = as_zmk_modifiers_state_changed(eh);
    if (m_ev != NULL) {
        return (struct mods_status_state){ .mods = (uint8_t)m_ev->modifiers };
    }

    return (struct mods_status_state){ .mods = mods };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mods_status, struct mods_status_state,
                            mods_status_update_cb, mods_status_get_state);

ZMK_SUBSCRIPTION(widget_mods_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_mods_status, zmk_modifiers_state_changed);

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
    mono_clear(widget->cbuf, COLOR_BLACK);

    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_remove_style_all(canvas);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CORNE_CANVAS_WIDTH, CORNE_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    widget_output_status_init();
    widget_layer_status_init();
    widget_mods_status_init();
#else
    widget_peripheral_status_init();
#endif

    draw_full_screen(widget);
    return 0;
}

lv_obj_t *zmk_widget_screen_status_obj(struct zmk_widget_screen_status *widget) {
    return widget->obj;
}
