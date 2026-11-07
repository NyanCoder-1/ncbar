#include <stdio.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "platform/wayland/context.h"
#include "platform/wayland/output-manager.h"
#include "wlr-layer-shell-unstable-v1.h"

static void wl_registry_on_global_listener(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void wl_registry_on_global_remove_listener(void *data, struct wl_registry *registry, uint32_t name);
static const struct wl_registry_listener wl_registry_listener = {.global = wl_registry_on_global_listener, .global_remove = wl_registry_on_global_remove_listener};

#define WL_OUTPUT_TARGET_VERSION 4
#define WL_OUTPUT_MIN_VERSION 4

enum ncbar_wayland_error wayland_context_init_registry(struct wayland_context *wl_context) {
    struct wl_registry *registry = wl_display_get_registry(wl_context->display);
    if (registry == NULL) {
        return NCBAR_WAYLAND_REGISTRY_OBTAIN_FAILED;
    }
    wl_context->registry = registry;
    if (wl_registry_add_listener(registry, &wl_registry_listener, wl_context) < 0) {
        return NCBAR_WAYLAND_REGISTRY_LISTENER_FAILED;
    }
    if (wl_display_roundtrip(wl_context->display) < 0) {
        return NCBAR_WAYLAND_DISPLAY_ROUNDTRIP_FAILED;
    }
    if (wl_context->compositor == NULL) {
        return NCBAR_WAYLAND_COMPOSITOR_OBTAIN_FAILED;
    }
    if (wl_context->layer_shell == NULL) {
        return NCBAR_WAYLAND_LAYER_SHELL_OBTAIN_FAILED;
    }
    return NCBAR_WAYLAND_OK;
}

static void wl_registry_on_global_listener(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (data) {
        struct wayland_context *wl_context = (struct wayland_context *)data;
        if (strcmp(interface, wl_compositor_interface.name) == 0) {
            wl_context->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
            wl_context->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
            if (version < WL_OUTPUT_MIN_VERSION) {
                fprintf(stderr, "Warning: wl_output version is less than %d (%d)\n", WL_OUTPUT_MIN_VERSION, version);
                return;
            }
            wayland_output_manager_add(wl_context->output_manager, registry, name, WL_OUTPUT_TARGET_VERSION);
        }
    }
}
static void wl_registry_on_global_remove_listener(void *data, struct wl_registry *registry, uint32_t name) {
    (void)registry;
    if (data) {
        struct wayland_context *wl_context = (struct wayland_context *)data;
        wayland_output_manager_remove(wl_context->output_manager, name);
    }
}