#ifndef PLATFORM_WAYLAND_OUTPUT_MANAGER_H
#define PLATFORM_WAYLAND_OUTPUT_MANAGER_H

#include <stdint.h>

struct wayland_output_manager;
struct wl_registry;

void wayland_output_manager_add(struct wayland_output_manager *output_manager, struct wl_registry *registry, uint32_t name, uint32_t version);
void wayland_output_manager_remove(struct wayland_output_manager *output_manager, uint32_t name);

#endif