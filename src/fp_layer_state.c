/* SPDX-License-Identifier: MIT
 * ZMK 0.3 compatibility: momentary holds and a volatile lock have separate state.
 * All layer holds in this keymap use fp_mo. No event recursion or delayed relock.
 */
#include <errno.h>
#include <limits.h>
#include <zmk/keymap.h>
#include "fp_layer_state.h"

static uint8_t held[FP_LAYER_COUNT];
static bool locked[FP_LAYER_COUNT];

static bool valid(uint32_t layer) { return layer > 0 && layer < FP_LAYER_COUNT; }

int fp_layer_hold_press(uint32_t layer) {
    if (!valid(layer)) return -EINVAL;
    if (held[layer] == UINT8_MAX) return -EOVERFLOW;
    int rc = zmk_keymap_layer_activate(layer);
    if (rc < 0) return rc;
    held[layer]++;
    return 0;
}

int fp_layer_hold_release(uint32_t layer) {
    if (!valid(layer) || held[layer] == 0) return -EINVAL;
    held[layer]--;
    if (held[layer] == 0 && !locked[layer]) return zmk_keymap_layer_deactivate(layer);
    return 0;
}

int fp_layer_lock_toggle(uint32_t layer) {
    /* Only the two approved layers can be locked. */
    if (layer != 3 && layer != 4) return -ENOTSUP;
    if (locked[layer]) {
        int rc = held[layer] ? 0 : zmk_keymap_layer_deactivate(layer);
        if (rc < 0) return rc;
        locked[layer] = false;
        return 0;
    }
    if (!zmk_keymap_layer_active(layer)) return -EINVAL;
    locked[layer] = true;
    return 0;
}

bool fp_layer_is_locked(uint32_t layer) {
    return valid(layer) && locked[layer];
}
