#!/bin/bash

# prepare folders
mkdir -p include src

# xdg-shell
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./include/xdg-shell.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./src/xdg-shell.c

# wlr-layer-shell-unstable-v1
wayland-scanner client-header /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml ./include/wlr-layer-shell-unstable-v1.h
wayland-scanner private-code /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml ./src/wlr-layer-shell-unstable-v1.c
