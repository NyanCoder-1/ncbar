#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>
#include <stddef.h>

#include "platform/wayland/context.h"

enum ncbar_app_error {
    NCBAR_APP_OK = 0,
    NCBAR_APP_SIGNALS_INIT_FAILED,
    NCBAR_APP_WAYLAND_CONTEXT_INIT_FAILED,
    NCBAR_APP_OUTPUTS_CREATE_FAILED,
    NCBAR_APP_OUTPUTS_GROW_CAPACITY_ERROR,
    NCBAR_APP_OUTPUTS_GROW_REALLOCATE_FAILED,

    /* app_run(): epoll setup and waiting */
    NCBAR_APP_EPOLL_CREATE_FAILED,
    NCBAR_APP_EPOLL_ADD_FAILED,
    NCBAR_APP_EPOLL_MOD_FAILED,
    NCBAR_APP_EPOLL_WAIT_FAILED,

    /* app_run(): Wayland connection */
    NCBAR_APP_WAYLAND_DISPLAY_NULL,
    NCBAR_APP_WAYLAND_DISPATCH_FAILED,
    NCBAR_APP_WAYLAND_DISPLAY_ERROR,
    NCBAR_APP_WAYLAND_FLUSH_FAILED,
    NCBAR_APP_WAYLAND_READ_FAILED,
    NCBAR_APP_WAYLAND_DISCONNECTED
};

struct app {
    struct wayland_context wl_context;
    bool running;

    struct app_output *outputs;
    size_t outputs_size;
    size_t outputs_capacity;
};

enum ncbar_app_error app_init(struct app *app);
void app_destroy(struct app *app);
enum ncbar_app_error app_run(struct app *app);

#endif