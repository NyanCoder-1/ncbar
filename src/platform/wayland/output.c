#include "platform/wayland/output.h"
#include "platform/wayland/context.h"
#include <string.h>
#include <wayland-client-protocol.h>

static void wl_output_geometry_listener(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                        const char *make, const char *model, int32_t transform);
static void wl_output_mode_listener(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
static void wl_output_done_listener(void *data, struct wl_output *wl_output);
static void wl_output_scale_listener(void *data, struct wl_output *wl_output, int32_t factor);
static void wl_output_name_listener(void *data, struct wl_output *wl_output, const char *name);
static void wl_output_description_listener(void *data, struct wl_output *wl_output, const char *description);
static const struct wl_output_listener wl_output_listener = {
    .geometry = wl_output_geometry_listener,
    .mode = wl_output_mode_listener,
    .done = wl_output_done_listener,
    .scale = wl_output_scale_listener,
    .name = wl_output_name_listener,
    .description = wl_output_description_listener,
};

int wayland_context_init_output(struct wl_output *output, struct wl_output_listener_data *listener_data, uint32_t name, wayland_output_callback callback, void *userdata) {
    listener_data->scale = 1;
    listener_data->callback = callback;
    listener_data->userdata = userdata;
    listener_data->initialized = false;
    listener_data->registry_name = name;
    int result = wl_output_add_listener(output, &wl_output_listener, listener_data);
    if (result < 0) {
        return NCBAR_WAYLAND_OUTPUT_LISTENER_FAILED;
    }
    return NCBAR_WAYLAND_OK;
}

static void wl_output_geometry_listener(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                        const char *make, const char *model, int32_t transform) {
    if (data != NULL) {
        struct wl_output_listener_data *listener_data = data;
        listener_data->x = x;
        listener_data->y = y;
        listener_data->physical_width = physical_width;
        listener_data->physical_height = physical_height;
    }
}
static void wl_output_mode_listener(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    if (data != NULL && (flags & WL_OUTPUT_MODE_CURRENT) != 0) {
        struct wl_output_listener_data *listener_data = data;
        listener_data->width = width;
        listener_data->height = height;
        listener_data->refresh = refresh;
    }
}
static void wl_output_done_listener(void *data, struct wl_output *wl_output) {
    if (data != NULL) {
        struct wl_output_listener_data *listener_data = data;
        if (listener_data->callback != NULL) {
            struct wayland_output_info info = {.output = wl_output, .registry_name = listener_data->registry_name, .name = listener_data->name};
            listener_data->callback(listener_data->initialized ? WAYLAND_OUTPUT_CHANGED : WAYLAND_OUTPUT_ADDED, &info, listener_data->userdata);
            listener_data->initialized = true;
        }
    }
}
static void wl_output_scale_listener(void *data, struct wl_output *wl_output, int32_t factor) {
    if (data != NULL) {
        struct wl_output_listener_data *listener_data = data;
        listener_data->scale = factor;
    }
}
static void wl_output_name_listener(void *data, struct wl_output *wl_output, const char *name) {
    if (data != NULL) {
        struct wl_output_listener_data *listener_data = data;
        strncpy(listener_data->name, name, WL_OUTPUT_LISTENER_DATA_NAME_LENGTH - 1);
        listener_data->name[WL_OUTPUT_LISTENER_DATA_NAME_LENGTH - 1] = 0;
    }
}
static void wl_output_description_listener(void *data, struct wl_output *wl_output, const char *description) {}