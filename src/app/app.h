#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>
#include <stddef.h>

#include "platform/wayland/context.h"

struct app {
    struct wayland_context wl_context;
    bool running;

    size_t output_count;
    struct app_output *outputs;
};

int app_init(struct app *app);
void app_destroy(struct app *app);
int app_run(struct app *app);

#endif