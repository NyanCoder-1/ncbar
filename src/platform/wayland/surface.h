#ifndef PLATFORM_WAYLAND_SURFACE_H
#define PLATFORM_WAYLAND_SURFACE_H

#include "wlr-layer-shell-unstable-v1.h"
#include <stdint.h>

struct wayland_context;
struct wl_output;
struct wl_surface;
struct wayland_surface;

enum wayland_surface_event { WAYLAND_SURFACE_CONFIGURED, WAYLAND_SURFACE_CLOSED };
typedef void (*wayland_surface_callback)(enum wayland_surface_event event, struct wayland_surface *surface, void *userdata);

enum wayland_surface_role { WAYLAND_SURFACE_ROLE_LAYER, WAYLAND_SURFACE_ROLE_POPUP };

struct wayland_layer_params {
    struct wl_output *output;
    enum zwlr_layer_shell_v1_layer layer;
    uint32_t anchor;
    uint32_t width, height; /* 0 = stretch along anchored edges */
    int32_t exclusive_zone;
    const char *ns;
};

struct wayland_surface *wayland_surface_create_layer(struct wayland_context *ctx, const struct wayland_layer_params *params, wayland_surface_callback cb, void *userdata);
/* later: */
// struct wayland_surface *wayland_surface_create_popup(struct wayland_context *ctx, struct wayland_surface *parent, const struct wayland_popup_params *params,
// wayland_surface_callback cb, void *userdata);

void wayland_surface_destroy(struct wayland_surface *surface);
struct wl_surface *wayland_surface_get_wl_surface(const struct wayland_surface *surface);
uint32_t wayland_surface_get_width(const struct wayland_surface *surface);
uint32_t wayland_surface_get_height(const struct wayland_surface *surface);

#endif