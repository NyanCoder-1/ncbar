#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "platform/wayland/context.h"
#include "platform/wayland/output.h"

struct wayland_output_wrapper {
    struct wl_output *output;
    struct wl_output_listener_data *listener_data;
    uint32_t name;
    uint32_t version;
};

struct wayland_output_manager {
    struct wayland_output_wrapper *outputs;
    size_t outputs_size;
    size_t outputs_capacity;
    wayland_output_callback callback;
    void *userdata;
};

#define WAYLAND_OUTPUT_MANAGER_INITIAL_SIZE 4
#define WL_OUTPUT_RELEASE_MIN_VERSION 3

enum ncbar_wayland_error wayland_context_init_output_manager(struct wayland_context *wl_context, wayland_output_callback callback, void *userdata) {
    struct wayland_output_manager *output_manager = (struct wayland_output_manager *)calloc(1, sizeof(struct wayland_output_manager));
    if (output_manager == NULL) {
        return NCBAR_WAYLAND_OUTPUT_MANAGER_INIT_FAILED;
    }
    output_manager->callback = callback;
    output_manager->userdata = userdata;
    wl_context->output_manager = output_manager;

    return NCBAR_WAYLAND_OK;
}

void wayland_context_destroy_output_manager(struct wayland_context *wl_context) {
    if (wl_context == NULL)
        return;

    if (wl_context->output_manager != NULL) {
        if (wl_context->output_manager->outputs != NULL) {
            for (size_t i = 0; i < wl_context->output_manager->outputs_size; i++) {
                if (wl_context->output_manager->outputs[i].version >= WL_OUTPUT_RELEASE_MIN_VERSION) {
                    wl_output_release(wl_context->output_manager->outputs[i].output);
                } else {
                    wl_proxy_destroy((struct wl_proxy *)wl_context->output_manager->outputs[i].output);
                }
                if (wl_context->output_manager->outputs[i].listener_data != NULL) {
                    free(wl_context->output_manager->outputs[i].listener_data);
                    wl_context->output_manager->outputs[i].listener_data = NULL;
                }
            }
            free(wl_context->output_manager->outputs);
            memset(wl_context->output_manager, 0, sizeof(struct wayland_output_manager));
        }
        free(wl_context->output_manager);
        wl_context->output_manager = NULL;
    }
}
void wayland_output_manager_add(struct wayland_output_manager *self, struct wl_registry *registry, uint32_t name, uint32_t version) {
    if (self == NULL)
        return;
    // init
    if (self->outputs == NULL || self->outputs_capacity == 0) {
        size_t outputs_initial_capacity = WAYLAND_OUTPUT_MANAGER_INITIAL_SIZE;
        struct wayland_output_wrapper *outputs_new = (struct wayland_output_wrapper *)malloc(sizeof(struct wayland_output_wrapper) * outputs_initial_capacity);
        if (outputs_new != NULL) {
            self->outputs = outputs_new;
            self->outputs_capacity = outputs_initial_capacity;
            self->outputs_size = 0;
        } else {
            return;
        }
    }
    // grow
    if (self->outputs_size == self->outputs_capacity) {
        size_t new_capacity = self->outputs_capacity == 0 ? WAYLAND_OUTPUT_MANAGER_INITIAL_SIZE : self->outputs_capacity * 2;
        if (new_capacity < self->outputs_capacity || new_capacity > SIZE_MAX / sizeof(*self->outputs)) {
            return;
        }
        struct wayland_output_wrapper *new_pointer = (struct wayland_output_wrapper *)realloc(self->outputs, sizeof(struct wayland_output_wrapper) * new_capacity);
        if (new_pointer == NULL) {
            return;
        }
        self->outputs = new_pointer;
        self->outputs_capacity = new_capacity;
    }
    // put to the end of array
    struct wl_output *output = wl_registry_bind(registry, name, &wl_output_interface, version);
    if (output != NULL) {
        struct wl_output_listener_data *listener_data = (struct wl_output_listener_data *)calloc(1, sizeof(struct wl_output_listener_data));
        if (listener_data == NULL)
            return;
        int result = wayland_context_init_output(output, listener_data, name, self->callback, self->userdata);
        if (result != NCBAR_WAYLAND_OK) {
            free(listener_data);
            return;
        }
        self->outputs[self->outputs_size++] = (struct wayland_output_wrapper){.output = output, .listener_data = listener_data, .name = name, .version = version};
    }
}
void wayland_output_manager_remove(struct wayland_output_manager *self, uint32_t name) {
    if (self == NULL)
        return;
    size_t delta = 0;
    for (size_t i = 0; i < self->outputs_size; i++) {
        if (self->outputs[i].name == name) {
            delta++;
            if (self->callback != NULL) {
                struct wayland_output_info info = {.output = self->outputs[i].output, .registry_name = self->outputs[i].name, .name = self->outputs[i].listener_data->name};
                self->callback(WAYLAND_OUTPUT_REMOVED, &info, self->userdata);
            }
            if (self->outputs[i].version >= WL_OUTPUT_RELEASE_MIN_VERSION) {
                wl_output_release(self->outputs[i].output);
            } else {
                wl_proxy_destroy((struct wl_proxy *)self->outputs[i].output);
            }
            if (self->outputs[i].listener_data != NULL) {
                free(self->outputs[i].listener_data);
                self->outputs[i].listener_data = NULL;
            }
        } else if (delta > 0) {
            self->outputs[i - delta] = self->outputs[i];
        }
    }
    if (delta > 0) {
        self->outputs_size -= delta;
    }
}