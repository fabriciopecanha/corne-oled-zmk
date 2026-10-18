#!/usr/bin/env python3
"""
Render exact 32x128 vertical OLED bitmapped layouts for Corne Left (Central) and Right (Peripheral).
Uses the actual bitmap data from modifiers_sym.c, images.c, and PixelOperatorMono.ttf.
"""

import os
import re
from PIL import Image, ImageDraw, ImageFont

BASE_DIR = '/Users/fabriciopecanha/zmk-nice-oled-custom/boards/shields/nice_oled'
OUT_DIR = '/Users/fabriciopecanha/.gemini/antigravity/brain/4bb95fbd-7974-4036-ad52-66f2d74349e6'
DOWNLOAD_DIR = '/Users/fabriciopecanha/Downloads/corne_oled_clean_v1/preview'
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(DOWNLOAD_DIR, exist_ok=True)

def get_bitmap_bytes(filepath, name):
    with open(filepath, 'r') as f:
        text = f.read()
    pattern = r'uint8_t\s+' + name + r'\[\]\s*=\s*\{([^}]+)\};'
    m = re.search(pattern, text)
    if not m:
        raise ValueError(f"Bitmap {name} not found in {filepath}")
    block = m.group(1)
    if '#endif' in block:
        block = block.split('#endif')[-1]
    raw = [int(x.strip(), 16) for x in block.split(',') if x.strip().startswith('0x')]
    return raw

def draw_bitmap(img, x0, y0, raw_bytes, w, h, invert_bits=False):
    stride = (w + 7) // 8
    # If raw_bytes includes 8 palette bytes at start, skip them
    if len(raw_bytes) == 8 + stride * h:
        data = raw_bytes[8:]
    elif len(raw_bytes) == stride * h:
        data = raw_bytes
    else:
        # take the last stride*h bytes
        data = raw_bytes[-(stride * h):]
    
    for y in range(h):
        for x in range(w):
            byte_idx = y * stride + (x // 8)
            bit_idx = 7 - (x % 8)
            val = (data[byte_idx] >> bit_idx) & 1
            if invert_bits:
                val = 1 - val
            if val:
                img.putpixel((x0 + x, y0 + y), 1)

# Load font
font_ttf = os.path.join(BASE_DIR, 'src/fonts/PixelOperatorMono.ttf')
font8 = ImageFont.truetype(font_ttf, 8)
font12 = ImageFont.truetype(font_ttf, 12)
font16 = ImageFont.truetype(font_ttf, 16)
font22 = ImageFont.truetype(font_ttf, 22)

# Load C bitmap arrays
mod_c = os.path.join(BASE_DIR, 'assets/modifiers_sym.c')
img_c = os.path.join(BASE_DIR, 'assets/images.c')

control_bytes = get_bitmap_bytes(mod_c, 'control_0_map')
shift_bytes = get_bitmap_bytes(mod_c, 'shift_0_map')
opt_bytes = get_bitmap_bytes(mod_c, 'opt_0_map')
cmd_bytes = get_bitmap_bytes(mod_c, 'cmd_0_map')

bt_bytes = get_bitmap_bytes(img_c, 'bt_map')
bt_nosig_bytes = get_bitmap_bytes(img_c, 'bt_no_signal_map')
usb_bytes = get_bitmap_bytes(img_c, 'usb_map')
bolt_bytes = get_bitmap_bytes(img_c, 'bolt_map')

# --- 1. RENDER LEFT SCREEN (Central) ---
left_img = Image.new('1', (32, 128), color=0)
draw_left = ImageDraw.Draw(left_img)

# Battery: y=1, font 16, centered
bat_text = "95%"
w_bat = font16.getlength(bat_text)
x_bat = int((32 - w_bat) / 2)
draw_left.text((x_bat, 1), bat_text, font=font16, fill=1)

# Output & Profile: y=18
# bt icon (12x15) at x=2, y=18 (invert_bits=True because 0 is white in bt_map palette)
draw_bitmap(left_img, 2, 18, bt_bytes, 12, 15, invert_bits=True)
# Profile digit "1" at x=17, y=17 in font 16
draw_left.text((17, 17), "1", font=font16, fill=1)

# Layer Section:
# Header "LAYER" at y=40, font 8, centered
w_hdr = font8.getlength("LAYER")
draw_left.text((int((32 - w_hdr) / 2), 40), "LAYER", font=font8, fill=1)

# Big Numeral "0" at y=49, font 22, centered
w_num = font22.getlength("0")
draw_left.text((int((32 - w_num) / 2), 49), "0", font=font22, fill=1)

# Layer Name "BASE" at y=69, font 8, centered
w_name = font8.getlength("BASE")
draw_left.text((int((32 - w_name) / 2), 69), "BASE", font=font8, fill=1)

# Modifiers: y=95..124 (2x2 grid, 14x14 icons, 2px spacing)
# Control at (1, 95)
draw_bitmap(left_img, 1, 95, control_bytes, 14, 14, invert_bits=False)
# Shift at (17, 95)
draw_bitmap(left_img, 17, 95, shift_bytes, 14, 14, invert_bits=False)
# Option at (1, 111)
draw_bitmap(left_img, 1, 111, opt_bytes, 14, 14, invert_bits=False)
# Command at (17, 111)
draw_bitmap(left_img, 17, 111, cmd_bytes, 14, 14, invert_bits=False)


# --- 2. RENDER RIGHT SCREEN (Peripheral) ---
right_img = Image.new('1', (32, 128), color=0)
draw_right = ImageDraw.Draw(right_img)

# Battery: y=1, font 16, centered
bat_r_text = "90%"
w_bat_r = font16.getlength(bat_r_text)
x_bat_r = int((32 - w_bat_r) / 2)
draw_right.text((x_bat_r, 1), bat_r_text, font=font16, fill=1)

# Link status: y=18
# bt icon (12x15) at x=2, y=18
draw_bitmap(right_img, 2, 18, bt_bytes, 12, 15, invert_bits=True)
# "OK" at x=16, y=20 in font 12
draw_right.text((16, 20), "OK", font=font12, fill=1)

# Static area: y=38..127 clean, zero animations, reserved for future extensions


# Save 1:1 logical images (32x128)
left_img.save(os.path.join(OUT_DIR, 'corne_left_oled_32x128.png'))
right_img.save(os.path.join(OUT_DIR, 'corne_right_oled_32x128.png'))
left_img.save(os.path.join(DOWNLOAD_DIR, 'corne_left_oled_32x128.png'))
right_img.save(os.path.join(DOWNLOAD_DIR, 'corne_right_oled_32x128.png'))

# Save scaled 8x images (256 x 1024) without anti-aliasing (NEAREST)
SCALE = 8
left_scaled = left_img.resize((32 * SCALE, 128 * SCALE), Image.Resampling.NEAREST)
right_scaled = right_img.resize((32 * SCALE, 128 * SCALE), Image.Resampling.NEAREST)

left_scaled.save(os.path.join(OUT_DIR, 'corne_left_oled_scaled.png'))
right_scaled.save(os.path.join(OUT_DIR, 'corne_right_oled_scaled.png'))
left_scaled.save(os.path.join(DOWNLOAD_DIR, 'corne_left_oled_scaled.png'))
right_scaled.save(os.path.join(DOWNLOAD_DIR, 'corne_right_oled_scaled.png'))

# Create Side-by-Side Presentation with Labels and Coordinate Rulers
canvas_w = 32 * SCALE * 2 + 180
canvas_h = 128 * SCALE + 90
side_by_side = Image.new('RGB', (canvas_w, canvas_h), color=(14, 16, 20))
draw_sbs = ImageDraw.Draw(side_by_side)

# Screen positions
x_left_pos = 90
x_right_pos = x_left_pos + 32 * SCALE + 40
y_screen_pos = 55

# Draw border frames around screens
draw_sbs.rectangle([x_left_pos - 2, y_screen_pos - 2, x_left_pos + 32 * SCALE + 1, y_screen_pos + 128 * SCALE + 1], outline=(60, 75, 95), width=2)
draw_sbs.rectangle([x_right_pos - 2, y_screen_pos - 2, x_right_pos + 32 * SCALE + 1, y_screen_pos + 128 * SCALE + 1], outline=(60, 75, 95), width=2)

side_by_side.paste(left_scaled.convert('RGB'), (x_left_pos, y_screen_pos))
side_by_side.paste(right_scaled.convert('RGB'), (x_right_pos, y_screen_pos))

# Labels
font_ui = ImageFont.truetype(font_ttf, 16)
font_label = ImageFont.truetype(font_ttf, 11)
draw_sbs.text((x_left_pos + 15, 18), "ESQUERDA (CENTRAL)", font=font_ui, fill=(0, 230, 180))
draw_sbs.text((x_right_pos + 10, 18), "DIREITA (PERIPHERAL)", font=font_ui, fill=(0, 200, 255))

# Coordinate indicators along the side
coords = [
    (1, "y=1: Bateria %"), 
    (18, "y=18: Conexao/Perfil"), 
    (40, "y=40: Header Camada"), 
    (49, "y=49: Numeral Camada"), 
    (69, "y=69: Nome Camada"), 
    (95, "y=95: Mods (Ctrl/Shf)"),
    (111, "y=111: Mods (Opt/Cmd)")
]

for y_val, label in coords:
    y_px = y_screen_pos + y_val * SCALE
    draw_sbs.line([15, y_px, x_left_pos - 8, y_px], fill=(50, 65, 80), width=1)
    draw_sbs.text((15, y_px - 14), label, font=font_label, fill=(160, 180, 200))

side_by_side.save(os.path.join(OUT_DIR, 'corne_oled_side_by_side.png'))
side_by_side.save(os.path.join(DOWNLOAD_DIR, 'corne_oled_side_by_side.png'))

print("All bitmapped previews generated and saved successfully!")
