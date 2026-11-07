#ifndef APP_OUTPUT_H
#define APP_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

struct app;
struct app_output {
    uint32_t registry_name;

    bool configured;
    bool dirty;
};

int app_render_dirty_outputs(struct app *app);

#endif