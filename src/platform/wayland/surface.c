#include "platform/wayland/surface.h"

#include <stdint.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>

#include "platform/wayland/context.h"
#include "wlr-layer-shell-unstable-v1.h"

struct wayland_surface {
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    uint32_t width;
    uint32_t height;
    wayland_surface_callback callback;
    void *userdata;
    enum wayland_surface_role role;
};

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial, uint32_t width, uint32_t height);
static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface);
static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

struct wayland_surface *wayland_surface_create_layer(struct wayland_context *wl_context, const struct wayland_layer_params *params, wayland_surface_callback callback,
                                                     void *userdata) {
    struct wayland_surface *self = calloc(1, sizeof(struct wayland_surface));
    if (self == NULL) {
        return NULL;
    }
    self->callback = callback;
    self->userdata = userdata;
    self->role = WAYLAND_SURFACE_ROLE_LAYER;
    self->width = params->width;   /* until the first configure says otherwise */
    self->height = params->height; /* until the first configure says otherwise */

    self->surface = wl_compositor_create_surface(wl_context->compositor);
    if (self->surface == NULL) {
        goto fail;
    }
    self->layer_surface = zwlr_layer_shell_v1_get_layer_surface(wl_context->layer_shell, self->surface, params->output, params->layer, params->ns == NULL ? TARGET : params->ns);
    if (self->layer_surface == NULL) {
        goto fail;
    }
    if (zwlr_layer_surface_v1_add_listener(self->layer_surface, &layer_surface_listener, self) < 0) {
        goto fail;
    }

    /* Width 0 + left/right anchors = "stretch across the output"; the compositor tells us the real width. */
    zwlr_layer_surface_v1_set_size(self->layer_surface, 0, params->height);
    zwlr_layer_surface_v1_set_anchor(self->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(self->layer_surface, (int32_t)params->height);

    wl_surface_commit(self->surface); /* buffer-less commit: asks the compositor for the first configure */
    return self;

fail:
    wayland_surface_destroy(self);
    return NULL;
}

void wayland_surface_destroy(struct wayland_surface *self) {
    if (self == NULL) {
        return;
    }
    /* The role object must go before the wl_surface it wraps. */
    if (self->layer_surface != NULL) {
        zwlr_layer_surface_v1_destroy(self->layer_surface);
    }
    if (self->surface != NULL) {
        wl_surface_destroy(self->surface);
    }
    free(self);
}

struct wl_surface *wayland_surface_get_wl_surface(const struct wayland_surface *self) { return self->surface; }
uint32_t wayland_surface_get_width(const struct wayland_surface *self) { return self->width; }
uint32_t wayland_surface_get_height(const struct wayland_surface *self) { return self->height; }

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
    struct wayland_surface *self = data;

    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
    if (width != 0) {
        self->width = width;
    }
    if (height != 0) {
        self->height = height;
    }
    if (self->callback != NULL) {
        self->callback(WAYLAND_SURFACE_CONFIGURED, self, self->userdata);
    }
}
static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface) {
    (void)layer_surface;
    struct wayland_surface *self = data;

    if (self->callback != NULL) {
        self->callback(WAYLAND_SURFACE_CLOSED, self, self->userdata); /* owner destroys; don't touch self after this */
    }
}