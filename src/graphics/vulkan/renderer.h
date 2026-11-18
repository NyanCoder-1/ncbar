#ifndef GRAPHICS_VULKAN_RENDERER_H
#define GRAPHICS_VULKAN_RENDERER_H

#include "graphics/vulkan/context.h"
#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

struct vulkan_context;

struct retire_queue_element {
    VkSwapchainKHR swapchain;
    VkImageView *image_views;
    VkFramebuffer *framebuffers;
};

struct vulkan_renderer {
    struct vulkan_context *context;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass render_pass;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    VkFramebuffer *swapchain_framebuffers;

    VkFormat swapchain_image_format;
    VkFormat render_pass_image_format;
    VkExtent2D swapchain_extent;

    VkSwapchainKHR *swapchain_retire_queue;
    VkImageView *swapchain_image_view_retire_queue;
    VkFramebuffer *swapchain_framebuffer_retire_queue;
    VkRenderPass *render_pass_retire_queue;

    bool swapchain_healthy;
};
struct vulkan_renderer_params {
    struct wl_display *wl_display;
    struct wl_surface *wl_surface;
    uint32_t width;
    uint32_t height;
};

struct vulkan_renderer *vulkan_renderer_init(struct vulkan_context *ctx, struct vulkan_renderer_params *params);
void vulkan_renderer_destroy(struct vulkan_renderer *vk_renderer);
enum ncbar_vulkan_error vulkan_renderer_resize(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height);

enum ncbar_vulkan_error vulkan_renderer_surface_create(struct vulkan_renderer *vk_renderer, struct wl_display *wl_display, struct wl_surface *wl_surface);
void vulkan_renderer_surface_destroy(struct vulkan_renderer *vk_renderer);
enum ncbar_vulkan_error vulkan_renderer_swapchain_create(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height);
void vulkan_renderer_swapchain_destroy(struct vulkan_renderer *vk_renderer);
enum ncbar_vulkan_error vulkan_renderer_render_pass_create(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_render_pass_destroy(struct vulkan_renderer *vk_renderer);
enum ncbar_vulkan_error vulkan_renderer_swapchain_image_views_create(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_swapchain_image_views_destroy(struct vulkan_renderer *vk_renderer);
enum ncbar_vulkan_error vulkan_renderer_swapchain_framebuffers_create(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_swapchain_framebuffers_destroy(struct vulkan_renderer *vk_renderer);
void vulkan_renderer_process_retirement(struct vulkan_renderer *vk_renderer, bool free_arrays);

#endif