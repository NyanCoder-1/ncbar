#include <stddef.h>
#include <wayland-client-core.h>

#include "platform/wayland/context.h"

int wayland_context_init_display(struct wayland_context *wl_context) {
    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        return NCBAR_WAYLAND_DISPLAY_INIT_FAILED;
    }
    wl_context->display = display;
    return NCBAR_WAYLAND_OK;
}

struct wl_display *wayland_context_get_display(struct wayland_context *context) { return context != NULL ? context->display : NULL; }