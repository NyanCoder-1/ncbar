#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <stdint.h>
#include <string.h>

static VkSurfaceCapabilitiesKHR get_surface_capabilities(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkSurfaceFormatKHR get_best_surface_format(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkPresentModeKHR get_best_present_mode(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkExtent2D get_best_swapchain_extent(VkSurfaceCapabilitiesKHR surface_capabilities, uint32_t window_width, uint32_t window_height);

enum ncbar_vulkan_error vulkan_renderer_swapchain_create(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR surface_capabilities = get_surface_capabilities(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    VkSurfaceFormatKHR best_surface_format = get_best_surface_format(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    VkPresentModeKHR best_present_mode = get_best_present_mode(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    VkExtent2D best_swapchain_extent = get_best_swapchain_extent(surface_capabilities, width, height);
    uint32_t image_array_layers = 1;

    VkSharingMode image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    const uint32_t queue_family_index_count = 0, *queue_family_index = NULL;

    VkSwapchainKHR swapchain_old = vk_renderer->swapchain;
    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_renderer->surface,
        .minImageCount = surface_capabilities.minImageCount + 1,
        .imageFormat = best_surface_format.format,
        .imageColorSpace = best_surface_format.colorSpace,
        .imageExtent = best_swapchain_extent,
        .imageArrayLayers = image_array_layers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = image_sharing_mode,
        .queueFamilyIndexCount = queue_family_index_count,
        .pQueueFamilyIndices = queue_family_index,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                                                                                                                   : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = best_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain_old,
    };
    if (swapchain_old != VK_NULL_HANDLE) {
        arrput(vk_renderer->retire_queue, swapchain_old);
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (vk_renderer->context->vk_loader_device.vkCreateSwapchainKHR(vk_renderer->context->device, &swapchain_create_info, VK_NULL_HANDLE, &swapchain) != VK_SUCCESS) {
        vk_renderer->swapchain = VK_NULL_HANDLE;
        return NCBAR_VULKAN_SWAPCHAIN_CREATE;
    }
    vk_renderer->swapchain = swapchain;
    return NCBAR_VULKAN_OK;
}
void vulkan_renderer_swapchain_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->surface == VK_NULL_HANDLE)
        return;
    vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->swapchain, NULL);
    vk_renderer->swapchain = VK_NULL_HANDLE;
}
void vulkan_renderer_process_retirement(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->surface == VK_NULL_HANDLE)
        return;
    for (ptrdiff_t i = 0; i < arrlen(vk_renderer->retire_queue); ++i) {
        vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->retire_queue[i], NULL);
        vk_renderer->retire_queue[i] = VK_NULL_HANDLE;
    }
    arrfree(vk_renderer->retire_queue);
}

static VkSurfaceCapabilitiesKHR get_surface_capabilities(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    vk_loader_instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities);
    return surface_capabilities;
}
static VkSurfaceFormatKHR get_best_surface_format(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t surface_format_number = 0;
    vk_loader_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_number, VK_NULL_HANDLE);
    VkSurfaceFormatKHR surface_formats[surface_format_number];
    memset(surface_formats, 0, sizeof(surface_formats));
    vk_loader_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_number, surface_formats);

    VkSurfaceFormatKHR best_surface_format = surface_formats[0];
    return best_surface_format;
}
static VkPresentModeKHR get_best_present_mode(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t present_mode_number = 0;
    vk_loader_instance.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_number, VK_NULL_HANDLE);
    VkPresentModeKHR present_modes[present_mode_number];
    memset(present_modes, 0, sizeof(present_modes));
    vk_loader_instance.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_number, present_modes);

    VkPresentModeKHR best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_mode_number; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            best_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return best_present_mode;
}
static VkExtent2D get_best_swapchain_extent(VkSurfaceCapabilitiesKHR surface_capabilities, uint32_t window_width, uint32_t window_height) {
    return (VkExtent2D){
        .width = surface_capabilities.currentExtent.width > window_width ? window_width : surface_capabilities.currentExtent.width,
        .height = surface_capabilities.currentExtent.height > window_height ? window_height : surface_capabilities.currentExtent.height,
    };
}