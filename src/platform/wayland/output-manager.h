#ifndef PLATFORM_WAYLAND_OUTPUT_MANAGER_H
#define PLATFORM_WAYLAND_OUTPUT_MANAGER_H

#include <stdint.h>

struct wayland_output_info;
struct wayland_output_manager;
struct wl_output;
struct wl_registry;

enum wayland_output_event { WAYLAND_OUTPUT_ADDED, WAYLAND_OUTPUT_CHANGED, WAYLAND_OUTPUT_REMOVED };

typedef void (*wayland_output_callback)(enum wayland_output_event event, const struct wayland_output_info *info, void *userdata);

void wayland_output_manager_add(struct wayland_output_manager *output_manager, struct wl_registry *registry, uint32_t name, uint32_t version);
void wayland_output_manager_remove(struct wayland_output_manager *output_manager, uint32_t name);

#endif