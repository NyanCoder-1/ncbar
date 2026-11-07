#include "app/app.h"
#include "platform/wayland/context.h"

#include <stdio.h>

static void app_on_wayland_output(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata);

int app_init(struct app *app) {
    if (wayland_context_init(&app->wl_context, app_on_wayland_output, app) != NCBAR_WAYLAND_OK) {
        return 1;
    }
    printf("wayland_context init success\n");
    app->running = true;
    return 0;
}
void app_destroy(struct app *app) { wayland_context_destroy(&app->wl_context); }

static void app_on_wayland_output(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata) {
    struct app *app = userdata;

    switch (event) {
    case WAYLAND_OUTPUT_ADDED:
        // Create an app_output for this monitor.
        break;

    case WAYLAND_OUTPUT_CHANGED:
        // Update the corresponding app_output.
        break;

    case WAYLAND_OUTPUT_REMOVED:
        // Destroy the corresponding app_output.
        break;
    }
}