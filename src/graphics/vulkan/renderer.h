#ifndef GRAPHICS_VULKAN_RENDERER_H
#define GRAPHICS_VULKAN_RENDERER_H

#include "graphics/vulkan/context.h"
#include <vulkan/vulkan_core.h>

struct vulkan_context;

struct vulkan_renderer {
    struct vulkan_context *context;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    VkSwapchainKHR *retire_queue;
};
struct vulkan_renderer_params {
    struct wl_display *wl_display;
    struct wl_surface *wl_surface;
    uint32_t width;
    uint32_t height;
};

struct vulkan_renderer *vulkan_renderer_init(struct vulkan_context *ctx, struct vulkan_renderer_params *params);
void vulkan_renderer_destroy(struct vulkan_renderer *vk_renderer);

enum ncbar_vulkan_error vulkan_renderer_surface_create(struct vulkan_renderer *vk_renderer, struct wl_display *wl_display, struct wl_surface *wl_surface);
enum ncbar_vulkan_error vulkan_renderer_swapchain_create(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height);
void vulkan_renderer_surface_destroy(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_swapchain_destroy(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_process_retirement(struct vulkan_renderer *vk_renderer);

#endif