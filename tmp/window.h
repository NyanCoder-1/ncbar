#ifndef WINDOW_H
#define WINDOW_H

#include "renderer.h"
#include "wayland-common.h"
#include "wlr-layer-shell-unstable-v1.h"
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

typedef struct Window {
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layerSurface;

    uint32_t width;
    uint32_t height;

    WaylandCommon *wlCommon;
    Renderer *renderer;
    VkSurfaceKHR vkSurface;
    VkSwapchainKHR vkSwapchain;

    bool configured : 1;
    bool closed : 1;
    bool resizePending : 1;
} Window;

bool Window_Create(Window *wnd, WaylandCommon *wlCommon, Renderer *renderer);
void Window_Destroy(Window *wnd);

#endif