#include "graphics/vulkan/renderer.h"

enum ncbar_vulkan_error vulkan_renderer_surface_create(struct vulkan_renderer *vk_renderer, struct wl_display *wl_display, struct wl_surface *wl_surface) {
    if (vk_renderer == NULL || wl_surface == NULL) {
        return NCBAR_VULKAN_SURFACE_INVALID_ARGUMENT;
    }
    const VkWaylandSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = wl_display,
        .surface = wl_surface,
    };
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = vk_renderer->context->vk_loader_instance.vkCreateWaylandSurfaceKHR(vk_renderer->context->instance, &create_info, NULL, &surface);
    if (result != VK_SUCCESS) {
        return NCBAR_VULKAN_SURFACE_CREATE_FAILED;
    }
    vk_renderer->surface = surface;
    return NCBAR_VULKAN_OK;
}
void vulkan_renderer_surface_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->surface == VK_NULL_HANDLE)
        return;
    vk_renderer->context->vk_loader_instance.vkDestroySurfaceKHR(vk_renderer->context->instance, vk_renderer->surface, NULL);
    vk_renderer->surface = VK_NULL_HANDLE;
}