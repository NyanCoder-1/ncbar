cmake_minimum_required (VERSION 3.8)

add_library(wlr-protocols STATIC src/xdg-shell.c src/wlr-layer-shell-unstable-v1.c)

target_include_directories(wlr-protocols PUBLIC include)
