#ifndef PLATFORM_WAYLAND_CONTEXT_H
#define PLATFORM_WAYLAND_CONTEXT_H

#include <stdint.h>

struct wayland_output_info {
    struct wl_output *output;
    uint32_t registry_name;
    const char *name;
};
enum wayland_output_event { WAYLAND_OUTPUT_ADDED, WAYLAND_OUTPUT_CHANGED, WAYLAND_OUTPUT_REMOVED };
typedef void (*wayland_output_callback)(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata);

struct wayland_context {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wayland_output_manager *output_manager;
};

enum ncbar_wayland_error {
    NCBAR_WAYLAND_OK = 0,
    NCBAR_WAYLAND_DISPLAY_INIT_FAILED,
    NCBAR_WAYLAND_DISPLAY_ROUNDTRIP_FAILED,
    NCBAR_WAYLAND_OUTPUT_MANAGER_INIT_FAILED,
    NCBAR_WAYLAND_REGISTRY_OBTAIN_FAILED,
    NCBAR_WAYLAND_REGISTRY_LISTENER_FAILED,
    NCBAR_WAYLAND_COMPOSITOR_OBTAIN_FAILED,
    NCBAR_WAYLAND_LAYER_SHELL_OBTAIN_FAILED,
    NCBAR_WAYLAND_OUTPUT_LISTENER_FAILED
};

enum ncbar_wayland_error wayland_context_init(struct wayland_context *wl_context, wayland_output_callback callback, void *userdata);
void wayland_context_destroy(struct wayland_context *wl_context);

enum ncbar_wayland_error wayland_context_init_display(struct wayland_context *wl_context);
enum ncbar_wayland_error wayland_context_init_registry(struct wayland_context *wl_context);
enum ncbar_wayland_error wayland_context_init_output_manager(struct wayland_context *wl_context, wayland_output_callback callback, void *userdata);
void wayland_context_destroy_output_manager(struct wayland_context *wl_context);

struct wl_display *wayland_context_get_display(struct wayland_context *context);

#endif