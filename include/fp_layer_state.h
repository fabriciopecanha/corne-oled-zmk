/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#define FP_LAYER_COUNT 7
int fp_layer_hold_press(uint32_t layer);
int fp_layer_hold_release(uint32_t layer);
int fp_layer_lock_toggle(uint32_t layer);
bool fp_layer_is_locked(uint32_t layer);
