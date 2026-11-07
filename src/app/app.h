#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>
#include <stddef.h>

#include "platform/wayland/context.h"

enum ncbar_app_error {
    NCBAR_APP_OK = 0,
    NCBAR_APP_WAYLAND_CONTEXT_INIT_FAILED,
    NCBAR_APP_OUTPUTS_CREATE_FAILED,
    NCBAR_APP_OUTPUTS_GROW_CAPACITY_ERROR,
    NCBAR_APP_OUTPUTS_GROW_REALLOCATE_FAILED
};

struct app {
    struct wayland_context wl_context;
    bool running;

    struct app_output *outputs;
    size_t outputs_size;
    size_t outputs_capacity;
};

int app_init(struct app *app);
void app_destroy(struct app *app);
int app_run(struct app *app);

#endif