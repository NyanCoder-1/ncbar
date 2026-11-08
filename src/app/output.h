#ifndef APP_OUTPUT_H
#define APP_OUTPUT_H

#include "app/app.h"
#include <stdbool.h>
#include <stdint.h>

struct app;
struct app_output {
    struct wayland_surface *surface;
    uint32_t registry_name;

    bool configured;
    bool dirty;
};

struct app_output *app_output_create(struct app *app, const struct wayland_output_info *info);
void app_output_destroy(struct app_output *output);
enum ncbar_app_error app_render_dirty_outputs(struct app *app);

#endif