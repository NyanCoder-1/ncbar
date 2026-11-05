#include "platform/wayland/context.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

int wayland_context_init(struct wayland_context *wl_context) {
    int error = wayland_context_init_display(wl_context);
    if (error == NCBAR_WAYLAND_OK) {
        error = wayland_context_init_output_manager(wl_context);
    }
    if (error == NCBAR_WAYLAND_OK) {
        error = wayland_context_init_registry(wl_context);
    }
    return error;
}
void wayland_context_destroy(struct wayland_context *wl_context) {
    wayland_context_destroy_output_manager(wl_context);
    if (wl_context->layer_shell != NULL) {
        wl_proxy_destroy((struct wl_proxy *)wl_context->layer_shell); // because I bind version 1
        wl_context->layer_shell = NULL;
    }
    if (wl_context->compositor != NULL) {
        wl_proxy_destroy((struct wl_proxy *)wl_context->compositor); // because I bind version 1
        wl_context->compositor = NULL;
    }
    if (wl_context->registry != NULL) {
        wl_registry_destroy(wl_context->registry);
        wl_context->registry = NULL;
    }
    if (wl_context->display != NULL) {
        wl_display_disconnect(wl_context->display);
        wl_context->display = NULL;
    }
}
