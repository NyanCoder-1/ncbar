#include "app/output.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "app/app.h"
#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include "platform/wayland/surface.h"
#include "wlr-layer-shell-unstable-v1.h"

static void app_output_on_surface_event(enum wayland_surface_event event, struct wayland_surface *surface, void *userdata);

#define APP_BAR_HEIGHT 32

struct app_output *app_output_create(struct app *app, const struct wayland_output_info *info) {
    struct app_output *output = calloc(1, sizeof(*output));
    if (output == NULL) {
        return NULL;
    }
    output->app = app;
    output->registry_name = info->registry_name;

    struct wayland_layer_params wl_params = {
        .output = info->output,
        .layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        .anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
        .height = APP_BAR_HEIGHT,
        .exclusive_zone = APP_BAR_HEIGHT,
    };
    output->surface = wayland_surface_create_layer(&app->wl_context, &wl_params, app_output_on_surface_event, output);
    if (output->surface == NULL) {
        goto CLEANUP_SURFACE;
    }
    return output;
CLEANUP_SURFACE:
    free(output);
    return NULL;
}

void app_output_destroy(struct app_output *output) {
    if (output == NULL) {
        return;
    }
    if (output->renderer != NULL) {
        vulkan_renderer_destroy(output->renderer);
    }
    if (output->surface != NULL) { /* NULL if the compositor already closed it */
        wayland_surface_destroy(output->surface);
    }
    free(output);
}

enum ncbar_app_error app_render_dirty_outputs(struct app *app) {
    enum ncbar_app_error status = NCBAR_APP_OK;

    for (size_t i = 0; i < app->outputs_size; ++i) {
        struct app_output *output = app->outputs[i];

        if (!output->configured || !output->dirty || output->renderer == NULL) {
            continue;
        }

        enum ncbar_vulkan_error error = vulkan_renderer_render(output->renderer);
        if (error == NCBAR_VULKAN_RENDER_SKIPPED) {
            continue;
        } else if (error != NCBAR_VULKAN_OK) {
            return NCBAR_APP_VULKAN_RENDER_FAILED;
        }

        output->dirty = false;
    }

    return status;
}

static void app_output_on_surface_event(enum wayland_surface_event event, struct wayland_surface *surface, void *userdata) {
    struct app_output *output = userdata;
    uint32_t width, height;

    switch (event) {
    case WAYLAND_SURFACE_CONFIGURED:
        width = wayland_surface_get_width(surface);
        height = wayland_surface_get_height(surface);
        if (width != 0 && height != 0) {
            if (output->renderer == NULL) {
                struct vulkan_renderer_params vk_params = {
                    .wl_display = output->app->wl_context.display,
                    .wl_surface = wayland_surface_get_wl_surface(surface),
                    .width = width,
                    .height = height,
                };
                output->renderer = vulkan_renderer_init(&output->app->vk_context, &vk_params);
            } else {
                enum ncbar_vulkan_error error = vulkan_renderer_recreate_swapchain(output->renderer, width, height);
                if (error != NCBAR_VULKAN_OK) {
                    fprintf(stderr, "app_output_on_surface_event: vulkan_renderer_resize failed with %d\n", (int)error);
                }
            }
        }
        output->configured = true;
        output->dirty = true;
        break;

    case WAYLAND_SURFACE_CLOSED:
        vulkan_renderer_destroy(output->renderer);
        output->renderer = NULL;
        wayland_surface_destroy(surface);
        output->surface = NULL;
        output->configured = false;
        break;
    }
}