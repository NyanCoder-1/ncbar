#include "app/output.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "app/app.h"
#include "platform/wayland/surface.h"
#include "wlr-layer-shell-unstable-v1.h"

static void app_output_on_surface_event(enum wayland_surface_event event, struct wayland_surface *surface, void *userdata);

#define APP_BAR_HEIGHT 32

struct app_output *app_output_create(struct app *app, const struct wayland_output_info *info) {
    struct app_output *output = calloc(1, sizeof(*output));
    if (output == NULL) {
        return NULL;
    }
    output->registry_name = info->registry_name;

    struct wayland_layer_params params = {
        .output = info->output,
        .layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        .anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
        .height = APP_BAR_HEIGHT,
        .exclusive_zone = APP_BAR_HEIGHT,
    };
    output->surface = wayland_surface_create_layer(&app->wl_context, &params, app_output_on_surface_event, output);
    if (output->surface == NULL) {
        free(output);
        return NULL;
    }
    return output;
}

void app_output_destroy(struct app_output *output) {
    if (output == NULL) {
        return;
    }
    /* later, in this order: renderer (swapchain, VkSurfaceKHR), then the Wayland surface */
    if (output->surface != NULL) { /* NULL if the compositor already closed it */
        wayland_surface_destroy(output->surface);
    }
    free(output);
}

enum ncbar_app_error app_render_dirty_outputs(struct app *app) {
    enum ncbar_app_error status = NCBAR_APP_OK;

    for (size_t i = 0; i < app->outputs_size; ++i) {
        struct app_output *output = app->outputs[i];

        if (!output->configured || !output->dirty) {
            continue;
        }

        output->dirty = false;
    }

    return status;
}

static void app_output_on_surface_event(enum wayland_surface_event event, struct wayland_surface *surface, void *userdata) {
    struct app_output *output = userdata;

    switch (event) {
    case WAYLAND_SURFACE_CONFIGURED:
        fprintf(stderr, "output %u configured: %ux%u\n", output->registry_name, wayland_surface_get_width(surface), wayland_surface_get_height(surface));
        /* later: create or resize the Vulkan swapchain here */
        output->configured = true;
        output->dirty = true;
        break;

    case WAYLAND_SURFACE_CLOSED:
        fprintf(stderr, "output %u closed\n", output->registry_name);
        /* later: destroy the renderer first */
        wayland_surface_destroy(surface);
        output->surface = NULL;
        output->configured = false;
        break;
    }
}