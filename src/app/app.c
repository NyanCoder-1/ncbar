#include "app/app.h"
#include "app/output.h"
#include "platform/wayland/context.h"

#include <stdlib.h>

static void app_on_wayland_output(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata);
static enum ncbar_app_error app_outputs_grow(struct app *self);
static void app_outputs_destroy(struct app *self);
static void app_add_output(struct app *self, const struct wayland_output_info *info);
static void app_remove_output(struct app *self, const struct wayland_output_info *info);

#define APP_OUTPUTS_INITIAL_SIZE 4

enum ncbar_app_error app_init(struct app *app) {
    if (wayland_context_init(&app->wl_context, app_on_wayland_output, app) != NCBAR_WAYLAND_OK) {
        return NCBAR_APP_WAYLAND_CONTEXT_INIT_FAILED;
    }
    app->running = true;
    return NCBAR_APP_OK;
}
void app_destroy(struct app *app) {
    app_outputs_destroy(app);
    wayland_context_destroy(&app->wl_context);
}

static void app_on_wayland_output(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata) {
    struct app *app = userdata;

    switch (event) {
    case WAYLAND_OUTPUT_ADDED:
        app_add_output(app, info);
        break;

    case WAYLAND_OUTPUT_CHANGED:
        // Update the corresponding app_output.
        break;

    case WAYLAND_OUTPUT_REMOVED:
        app_remove_output(app, info);
        break;
    }
}
static void app_add_output(struct app *self, const struct wayland_output_info *info) {
    if (app_outputs_grow(self) != NCBAR_APP_OK)
        return;

    self->outputs[self->outputs_size++] = (struct app_output){.registry_name = info->registry_name, .configured = false, .dirty = true};
}
static void app_remove_output(struct app *self, const struct wayland_output_info *info) {
    size_t delta = 0;
    for (size_t i = 0; i < self->outputs_size; i++) {
        if (self->outputs[i].registry_name == info->registry_name) {
            delta++;
        } else if (delta > 0) {
            self->outputs[i - delta] = self->outputs[i];
        }
    }
    if (delta > 0) {
        self->outputs_size -= delta;
    }
}
static enum ncbar_app_error app_outputs_grow(struct app *self) {
    if (self->outputs_size == self->outputs_capacity) {
        size_t new_capacity = self->outputs_capacity == 0 ? APP_OUTPUTS_INITIAL_SIZE : self->outputs_capacity * 2;
        if (new_capacity < self->outputs_capacity || new_capacity > SIZE_MAX / sizeof(struct app_output)) {
            return NCBAR_APP_OUTPUTS_GROW_CAPACITY_ERROR;
        }
        struct app_output *new_pointer = (struct app_output *)realloc(self->outputs, sizeof(struct app_output) * new_capacity);
        if (new_pointer == NULL) {
            return NCBAR_APP_OUTPUTS_GROW_REALLOCATE_FAILED;
        }
        self->outputs = new_pointer;
        self->outputs_capacity = new_capacity;
    }

    return NCBAR_APP_OK;
}
static void app_outputs_destroy(struct app *self) {
    if (self->outputs != NULL) {
        free(self->outputs);
    }
    self->outputs = NULL;
    self->outputs_capacity = 0;
    self->outputs_size = 0;
}