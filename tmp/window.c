#include "window.h"
#include "renderer.h"
#include "wayland-common.h"
#include "wlr-layer-shell-unstable-v1.h"
#include <string.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

static void layer_surface_configure(void *data,
                                    struct zwlr_layer_surface_v1 *layer_surface,
                                    uint32_t serial, uint32_t width,
                                    uint32_t height);
static void layer_surface_closed(void *data,
                                 struct zwlr_layer_surface_v1 *layer_surface);

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

bool Window_Create(Window *wnd, WaylandCommon *wlCommon, Renderer *renderer) {
    WaylandWindowData wndData = WaylandCommon_CreateWindow(
        wlCommon, "ncbar", ZWLR_LAYER_SHELL_V1_LAYER_TOP);
    if (wndData.layerSurface == NULL || wndData.surface == NULL) {
        return false;
    }
    memset(wnd, 0, sizeof(Window));
    zwlr_layer_surface_v1_set_anchor(wndData.layerSurface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    const uint32_t bar_height = 40;
    zwlr_layer_surface_v1_set_size(wndData.layerSurface, 0, bar_height);
    zwlr_layer_surface_v1_set_exclusive_zone(wndData.layerSurface,
                                             (int32_t)bar_height);
    wnd->surface = wndData.surface;
    wnd->layerSurface = wndData.layerSurface;
    wnd->wlCommon = wlCommon;
    wnd->renderer = renderer;
    zwlr_layer_surface_v1_add_listener(wnd->layerSurface,
                                       &layer_surface_listener, wnd);
    wl_surface_commit(wndData.surface);
    wl_display_roundtrip(wlCommon->display);
    if (!wnd->configured) {
        wl_surface_destroy(wnd->surface);
        memset(wnd, 0, sizeof(Window));
        return false;
    }
    return true;
}
void Window_Destroy(Window *wnd) {
    if (wnd->vkSwapchain != VK_NULL_HANDLE) {
        wnd->renderer->vkWrapper.vkDestroySwapchainKHR(wnd->renderer->device,
                                                       wnd->vkSwapchain, NULL);
    }
    if (wnd->vkSurface != VK_NULL_HANDLE) {
        wnd->renderer->vkWrapper.vkDestroySurfaceKHR(wnd->renderer->instance,
                                                     wnd->vkSurface, NULL);
    }
    if (wnd->layerSurface != NULL) {
        zwlr_layer_surface_v1_destroy(wnd->layerSurface);
    }
    if (wnd->surface != NULL) {
        wl_surface_destroy(wnd->surface);
    }
    memset(wnd, 0, sizeof(Window));
}

static void layer_surface_configure(void *data,
                                    struct zwlr_layer_surface_v1 *layer_surface,
                                    uint32_t serial, uint32_t width,
                                    uint32_t height) {
    Window *window = data;

    window->width = width;
    window->height = height;
    window->resizePending = true;

    // Always ack the configure
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

    if (window->vkSurface == VK_NULL_HANDLE) {
        VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
        if (!Renderer_CreateSurface(window->renderer, &vkSurface,
                                    window->wlCommon->display,
                                    window->surface)) {
            return;
        }
        window->vkSurface = vkSurface;
    }

    if (!Renderer_CreateSwapChain(window->renderer, &window->vkSwapchain,
                                  window->vkSurface, width, height)) {
        return;
    }

    window->configured = true;
}
static void layer_surface_closed(void *data,
                                 struct zwlr_layer_surface_v1 *layer_surface) {
    (void)layer_surface;
    Window *wnd = data;
    wnd->closed = true;
}