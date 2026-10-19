# Copyright (c) 2026 Fabricio Pecanha
# SPDX-License-Identifier: MIT

if(CONFIG_SHIELD_CORNE_OLED)
    target_sources(app PRIVATE
        custom_status_screen.c
        widgets/screen_status.c
        widgets/mono_draw.c
    )
    target_include_directories(app PRIVATE . widgets)
endif()
