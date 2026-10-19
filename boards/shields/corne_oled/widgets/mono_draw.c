/*
 * Copyright (c) 2026 Fabricio Pecanha
 * SPDX-License-Identifier: MIT
 */

#include "mono_draw.h"
#include <string.h>
#include <ctype.h>

void mono_clear(lv_color_t *cbuf, lv_color_t color) {
    memset(cbuf, color.full ? 0xFF : 0x00, CORNE_CANVAS_PIXELS * sizeof(lv_color_t));
}

void mono_draw_pixel(lv_color_t *cbuf, int x, int y, lv_color_t color) {
    if (x >= 0 && x < CORNE_LOGICAL_WIDTH && y >= 0 && y < CORNE_LOGICAL_HEIGHT) {
        cbuf[x * CORNE_CANVAS_WIDTH + (127 - y)] = color;
    }
}

void mono_draw_line_h(lv_color_t *cbuf, int x0, int x1, int y, lv_color_t color) {
    if (y < 0 || y >= CORNE_LOGICAL_HEIGHT) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 >= CORNE_LOGICAL_WIDTH) x1 = CORNE_LOGICAL_WIDTH - 1;
    for (int x = x0; x <= x1; x++) {
        cbuf[x * CORNE_CANVAS_WIDTH + (127 - y)] = color;
    }
}

void mono_draw_line_v(lv_color_t *cbuf, int x, int y0, int y1, lv_color_t color) {
    if (x < 0 || x >= CORNE_LOGICAL_WIDTH) return;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (y0 < 0) y0 = 0;
    if (y1 >= CORNE_LOGICAL_HEIGHT) y1 = CORNE_LOGICAL_HEIGHT - 1;
    int row_offset = x * CORNE_CANVAS_WIDTH;
    for (int y = y0; y <= y1; y++) {
        cbuf[row_offset + (127 - y)] = color;
    }
}

void mono_draw_rect(lv_color_t *cbuf, int x, int y, int w, int h, lv_color_t color) {
    if (w <= 0 || h <= 0) return;
    mono_draw_line_h(cbuf, x, x + w - 1, y, color);
    mono_draw_line_h(cbuf, x, x + w - 1, y + h - 1, color);
    mono_draw_line_v(cbuf, x, y, y + h - 1, color);
    mono_draw_line_v(cbuf, x + w - 1, y, y + h - 1, color);
}

void mono_fill_rect(lv_color_t *cbuf, int x, int y, int w, int h, lv_color_t color) {
    if (w <= 0 || h <= 0) return;
    for (int cur_y = y; cur_y < y + h; cur_y++) {
        mono_draw_line_h(cbuf, x, x + w - 1, cur_y, color);
    }
}

// -----------------------------------------------------------------------------
// 4x6 Micro Font Table (ASCII 32 ' ' to 90 'Z' + '%')
// Each glyph is 6 rows of 4 bits packed into 3 bytes (4 bits per row = 24 bits = 3 bytes)
// -----------------------------------------------------------------------------
static const uint8_t font_4x6_glyphs[][3] = {
    // '0' (idx 0)
    {0x69, 0x99, 0x96},
    // '1' (idx 1)
    {0x26, 0x22, 0x27},
    // '2' (idx 2)
    {0x69, 0x12, 0x4F},
    // '3' (idx 3)
    {0xE1, 0x61, 0x1E},
    // '4' (idx 4)
    {0x99, 0xF1, 0x11},
    // '5' (idx 5)
    {0xF8, 0xE1, 0x1E},
    // '6' (idx 6)
    {0x68, 0xE9, 0x96},
    // '7' (idx 7)
    {0xF1, 0x24, 0x44},
    // '8' (idx 8)
    {0x69, 0x69, 0x96},
    // '9' (idx 9)
    {0x69, 0x97, 0x16},
    // 'A' (idx 10)
    {0x69, 0xF9, 0x99},
    // 'B' (idx 11)
    {0xE9, 0xE9, 0x9E},
    // 'C' (idx 12)
    {0x78, 0x88, 0x87},
    // 'D' (idx 13)
    {0xE9, 0x99, 0x9E},
    // 'E' (idx 14)
    {0xF8, 0xE8, 0x8F},
    // 'F' (idx 15)
    {0xF8, 0xE8, 0x88},
    // 'G' (idx 16)
    {0x78, 0xB9, 0x97},
    // 'H' (idx 17)
    {0x99, 0xF9, 0x99},
    // 'I' (idx 18)
    {0xE4, 0x44, 0x4E},
    // 'J' (idx 19)
    {0x11, 0x11, 0x96},
    // 'K' (idx 20)
    {0x9A, 0xCA, 0x99},
    // 'L' (idx 21)
    {0x88, 0x88, 0x8F},
    // 'M' (idx 22)
    {0x9F, 0x99, 0x99},
    // 'N' (idx 23)
    {0x9D, 0xB9, 0x99},
    // 'O' (idx 24)
    {0x69, 0x99, 0x96},
    // 'P' (idx 25)
    {0xE9, 0xE8, 0x88},
    // 'Q' (idx 26)
    {0x69, 0x99, 0x61},
    // 'R' (idx 27)
    {0xE9, 0xEA, 0x99},
    // 'S' (idx 28)
    {0x78, 0x61, 0x1E},
    // 'T' (idx 29)
    {0xF4, 0x44, 0x44},
    // 'U' (idx 30)
    {0x99, 0x99, 0x96},
    // 'V' (idx 31)
    {0x99, 0x99, 0x66},
    // 'W' (idx 32)
    {0x99, 0x99, 0xF9},
    // 'X' (idx 33)
    {0x99, 0x66, 0x99},
    // 'Y' (idx 34)
    {0x99, 0x64, 0x44},
    // 'Z' (idx 35)
    {0xF1, 0x24, 0x8F},
    // '%' (idx 36)
    {0x92, 0x42, 0x90},
    // '-' (idx 37)
    {0x00, 0xF0, 0x00},
    // '.' (idx 38)
    {0x00, 0x00, 0x66},
    // ' ' (idx 39)
    {0x00, 0x00, 0x00},
};

static int get_glyph_index(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return (c - 'A') + 10;
    if (c >= 'a' && c <= 'z') return (c - 'a') + 10;
    if (c == '%') return 36;
    if (c == '-') return 37;
    if (c == '.') return 38;
    return 39; // space
}

void mono_draw_char(lv_color_t *cbuf, int x, int y, char ch, lv_color_t color) {
    int idx = get_glyph_index(ch);
    const uint8_t *g = font_4x6_glyphs[idx];
    for (int r = 0; r < 6; r++) {
        uint8_t nibble;
        if ((r % 2) == 0) {
            nibble = (g[r / 2] >> 4) & 0x0F;
        } else {
            nibble = g[r / 2] & 0x0F;
        }
        for (int c = 0; c < 4; c++) {
            if ((nibble >> (3 - c)) & 1) {
                mono_draw_pixel(cbuf, x + c, y + r, color);
            }
        }
    }
}

void mono_draw_string(lv_color_t *cbuf, int x, int y, const char *str, lv_color_t color) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        if (cur_x + 4 > CORNE_LOGICAL_WIDTH) break;
        mono_draw_char(cbuf, cur_x, y, *str, color);
        cur_x += 5; // 4px glyph + 1px spacing
        str++;
    }
}

// -----------------------------------------------------------------------------
// Big Digit (14 wide x 22 high, thickness 3)
// -----------------------------------------------------------------------------
void mono_draw_big_digit(lv_color_t *cbuf, int x, int y, int digit, lv_color_t color) {
    const int w = 14;
    const int h = 22;
    const int t = 3;

    if (digit == 0) {
        mono_fill_rect(cbuf, x, y, w, h, color);
        mono_fill_rect(cbuf, x + t, y + t, w - 2 * t, h - 2 * t, COLOR_BLACK);
    } else if (digit == 1) {
        mono_fill_rect(cbuf, x + w / 2 - 1, y, t, h, color);
        mono_fill_rect(cbuf, x + w / 2 - 4, y, 4, t, color);
        mono_fill_rect(cbuf, x + 2, y + h - t, w - 4, t, color);
    } else if (digit == 2) {
        mono_fill_rect(cbuf, x, y, w, t, color);
        mono_fill_rect(cbuf, x + w - t, y, t, h / 2, color);
        mono_fill_rect(cbuf, x, y + h / 2 - t / 2, w, t, color);
        mono_fill_rect(cbuf, x, y + h / 2, t, h / 2, color);
        mono_fill_rect(cbuf, x, y + h - t, w, t, color);
    } else if (digit == 3) {
        mono_fill_rect(cbuf, x, y, w, t, color);
        mono_fill_rect(cbuf, x + w - t, y, t, h, color);
        mono_fill_rect(cbuf, x, y + h / 2 - t / 2, w, t, color);
        mono_fill_rect(cbuf, x, y + h - t, w, t, color);
    } else {
        mono_draw_rect(cbuf, x, y, w, h, color);
    }
}

void mono_draw_battery(lv_color_t *cbuf, int x, int y, uint8_t level, bool charging) {
    // Battery box outline (w=11, h=6)
    mono_draw_rect(cbuf, x, y, 10, 6, COLOR_WHITE);
    mono_draw_line_v(cbuf, x + 10, y + 2, y + 3, COLOR_WHITE); // nipple
    if (charging) {
        // Draw '+' charging indicator
        mono_draw_line_h(cbuf, x + 3, x + 7, y + 2, COLOR_WHITE);
        mono_draw_line_v(cbuf, x + 5, y + 1, y + 4, COLOR_WHITE);
    } else if (level > 0) {
        int fill_w = (level * 7) / 100;
        if (fill_w < 1) fill_w = 1;
        if (fill_w > 7) fill_w = 7;
        mono_fill_rect(cbuf, x + 1, y + 1, fill_w, 4, COLOR_WHITE);
    }
}
