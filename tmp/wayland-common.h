#ifndef WAYLAND_COMMON_H
#define WAYLAND_COMMON_H

#include "wlr-layer-shell-unstable-v1.h"
#include <stdint.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

typedef struct WaylandCommon {
    uint8_t success;
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layerShell;
} WaylandCommon;

typedef struct WaylandWindowData {
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layerSurface;
} WaylandWindowData;

WaylandCommon WaylandCommon_Create();
void WaylandCommon_Destroy(WaylandCommon *wlCommon);
WaylandWindowData WaylandCommon_CreateWindow(WaylandCommon *self, const char *layerNamespace, uint32_t layerZIndex);

#endif