#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>
#include <stddef.h>

#include "platform/wayland/context.h"

struct app {
    struct wayland_context wl_context;
    bool running;

    struct app_output *outputs;
    size_t outputs_count;
    size_t outputs_capacity;
};

int app_init(struct app *app);
void app_destroy(struct app *app);
int app_run(struct app *app);

#endif