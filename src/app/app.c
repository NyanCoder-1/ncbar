#include "app/app.h"

#include <stdio.h>

#include "platform/wayland/context.h"

int app_init(struct app *app) {
    if (wayland_context_init(&app->wl_context) != NCBAR_WAYLAND_OK) {
        return 1;
    }
    printf("wayland_context init success\n");
    app->running = true;
    return 0;
}
void app_destroy(struct app *app) {
    wayland_context_destroy(&app->wl_context);
}