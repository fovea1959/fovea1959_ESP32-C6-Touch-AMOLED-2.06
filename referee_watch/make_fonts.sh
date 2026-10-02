#!/bin/bash
npx github:lvgl/lv_font_conv \
	--format lvgl --no-compress --stride 1 --align 1 --bpp 4 --font ../fonts/IBM_Plex_Mono/IBMPlexMono-Bold.ttf --size 96 --symbols " 0123456789.-+:" -o IBMPlexMonoBold_96.c

npx github:lvgl/lv_font_conv \
	--format lvgl --no-compress --stride 1 --align 1 --bpp 4 --font ../fonts/IBM_Plex_Mono/IBMPlexMono-Bold.ttf --size 60 --symbols " 0123456789.-+:" -o IBMPlexMonoBold_60.c
