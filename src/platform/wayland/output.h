#ifndef PLATFORM_WAYLAND_OUTPUT_H
#define PLATFORM_WAYLAND_OUTPUT_H

#include "platform/wayland/context.h"
#include <stdbool.h>
#include <wayland-client-protocol.h>

#define WL_OUTPUT_LISTENER_DATA_NAME_LENGTH 512
struct wl_output_listener_data {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t physical_width;
    int32_t physical_height;
    int32_t scale;
    int32_t refresh;
    uint32_t registry_name;
    char name[WL_OUTPUT_LISTENER_DATA_NAME_LENGTH];

    wayland_output_callback callback;
    void *userdata;
    bool initialized;
};
enum ncbar_wayland_error wayland_context_init_output(struct wl_output *output, struct wl_output_listener_data *listener_data, uint32_t name, wayland_output_callback callback,
                                                     void *userdata);

#endif