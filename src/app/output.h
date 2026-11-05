#ifndef APP_OUTPUT_H
#define APP_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

struct app;
struct app_output {
    uint64_t wayland_output_id;
    struct wl_output_window *window;
    struct vk_output *renderer;
    bool configured;
    bool dirty;
};

int app_render_dirty_outputs(struct app *app);

#endif