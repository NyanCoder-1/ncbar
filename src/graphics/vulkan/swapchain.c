#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

static VkSurfaceCapabilitiesKHR get_surface_capabilities(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkSurfaceFormatKHR get_best_surface_format(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkPresentModeKHR get_best_present_mode(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkExtent2D get_best_swapchain_extent(VkSurfaceCapabilitiesKHR surface_capabilities, uint32_t window_width, uint32_t window_height);
static VkCompositeAlphaFlagBitsKHR get_composite_alpha(VkCompositeAlphaFlagsKHR supported);

enum ncbar_vulkan_error vulkan_renderer_swapchain_create(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR surface_capabilities = get_surface_capabilities(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    VkSurfaceFormatKHR best_surface_format = get_best_surface_format(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    if (best_surface_format.format == VK_FORMAT_UNDEFINED) {
        return NCBAR_VULKAN_SURFACE_FORMAT_EMPTY;
    }
    VkPresentModeKHR best_present_mode = get_best_present_mode(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    VkExtent2D best_swapchain_extent = get_best_swapchain_extent(surface_capabilities, width, height);
    uint32_t image_array_layers = 1;

    VkSharingMode image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    const uint32_t queue_family_index_count = 0, *queue_family_index = NULL;

    VkSwapchainKHR swapchain_old = vk_renderer->swapchain;
    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount != 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }
    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_renderer->surface,
        .minImageCount = image_count,
        .imageFormat = best_surface_format.format,
        .imageColorSpace = best_surface_format.colorSpace,
        .imageExtent = best_swapchain_extent,
        .imageArrayLayers = image_array_layers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = image_sharing_mode,
        .queueFamilyIndexCount = queue_family_index_count,
        .pQueueFamilyIndices = queue_family_index,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = get_composite_alpha(surface_capabilities.supportedCompositeAlpha),
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
    if (vk_renderer == NULL || vk_renderer->swapchain == VK_NULL_HANDLE)
        return;
    vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->swapchain, NULL);
    vk_renderer->swapchain = VK_NULL_HANDLE;
}
void vulkan_renderer_process_retirement(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL)
        return;
    for (ptrdiff_t i = 0; i < arrlen(vk_renderer->retire_queue); ++i) {
        if (vk_renderer->retire_queue[i] != VK_NULL_HANDLE) {
            vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->retire_queue[i], NULL);
            vk_renderer->retire_queue[i] = VK_NULL_HANDLE;
        }
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
    VkResult result = vk_loader_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_number, VK_NULL_HANDLE);
    if (result != VK_SUCCESS || surface_format_number == 0) {
        return (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};
    }
    VkSurfaceFormatKHR surface_formats[surface_format_number];
    memset(surface_formats, 0, sizeof(surface_formats));
    vk_loader_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_number, surface_formats);
    if (result != VK_SUCCESS) {
        return (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};
    }

    const VkFormat preferred_formats[] = {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UINT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UINT,
    };
    const size_t preferred_formats_count = sizeof(preferred_formats) / sizeof(preferred_formats[0]);
    VkSurfaceFormatKHR best_surface_format = (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};
    for (size_t n = 0; n < preferred_formats_count; n++) {
        for (uint32_t i = 0; i < surface_format_number; i++) {
            if (surface_formats[i].format == preferred_formats[n]) {
                best_surface_format = surface_formats[i];
            }
        }
    }
    if (best_surface_format.format == VK_FORMAT_UNDEFINED) {
        best_surface_format = surface_formats[0];
    }

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

static uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max) { return value < min ? min : value > max ? max : value; }
static VkExtent2D get_best_swapchain_extent(VkSurfaceCapabilitiesKHR surface_capabilities, uint32_t window_width, uint32_t window_height) {
    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        return surface_capabilities.currentExtent;
    }

    return (VkExtent2D){
        .width = clamp_u32(window_width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
        .height = clamp_u32(window_height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height),
    };
}
static VkCompositeAlphaFlagBitsKHR get_composite_alpha(VkCompositeAlphaFlagsKHR supported) {
    const VkCompositeAlphaFlagBitsKHR modes[] = {VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        if ((supported & modes[i]) != 0) {
            return modes[i];
        }
    }

    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}