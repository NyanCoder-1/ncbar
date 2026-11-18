#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

struct surface_capabilities_result {
    VkSurfaceCapabilitiesKHR capabilities;
    enum ncbar_vulkan_error error;
};

static struct surface_capabilities_result get_surface_capabilities(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkSurfaceFormatKHR get_best_surface_format(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkPresentModeKHR get_best_present_mode(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface);
static VkExtent2D get_best_swapchain_extent(VkSurfaceCapabilitiesKHR surface_capabilities, uint32_t window_width, uint32_t window_height);
static VkCompositeAlphaFlagBitsKHR get_composite_alpha(VkCompositeAlphaFlagsKHR supported);
static enum ncbar_vulkan_error vulkan_renderer_swapchain_acquire_images(PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR, VkDevice device, VkSwapchainKHR swapchain,
                                                                        VkImage **swapchain_images_arr_ptr);

enum ncbar_vulkan_error vulkan_renderer_swapchain_create(struct vulkan_renderer *vk_renderer, uint32_t width, uint32_t height) {
    const struct surface_capabilities_result surface_capabilities_result =
        get_surface_capabilities(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    if (surface_capabilities_result.error != NCBAR_VULKAN_OK) {
        return surface_capabilities_result.error;
    }
    VkSurfaceCapabilitiesKHR surface_capabilities = surface_capabilities_result.capabilities;
    VkSurfaceFormatKHR best_surface_format = get_best_surface_format(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    if (best_surface_format.format == VK_FORMAT_UNDEFINED) {
        return NCBAR_VULKAN_SURFACE_FORMAT_EMPTY;
    }
    VkPresentModeKHR best_present_mode = get_best_present_mode(vk_renderer->context->vk_loader_instance, vk_renderer->context->physical_device, vk_renderer->surface);
    if (best_present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        return NCBAR_VULKAN_PRESENT_MODE_ERROR;
    }
    VkExtent2D best_swapchain_extent = get_best_swapchain_extent(surface_capabilities, width, height);
    uint32_t image_array_layers = 1;

    VkSharingMode image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    const uint32_t queue_family_index_count = 0, *queue_family_index = NULL;

    VkSwapchainKHR swapchain_old = vk_renderer->swapchain;
    uint32_t min_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount != 0 && min_image_count > surface_capabilities.maxImageCount) {
        min_image_count = surface_capabilities.maxImageCount;
    }
    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_renderer->surface,
        .minImageCount = min_image_count,
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
        arrput(vk_renderer->swapchain_retire_queue, swapchain_old);
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkResult result = vk_renderer->context->vk_loader_device.vkCreateSwapchainKHR(vk_renderer->context->device, &swapchain_create_info, VK_NULL_HANDLE, &swapchain);
    if (result != VK_SUCCESS) {
        vk_renderer->swapchain = VK_NULL_HANDLE;
        return NCBAR_VULKAN_SWAPCHAIN_CREATE;
    }
    vk_renderer->swapchain = swapchain;
    vk_renderer->swapchain_image_format = best_surface_format.format;
    vk_renderer->swapchain_extent = best_swapchain_extent;

    enum ncbar_vulkan_error error = vulkan_renderer_swapchain_acquire_images(vk_renderer->context->vk_loader_device.vkGetSwapchainImagesKHR, vk_renderer->context->device,
                                                                             swapchain, &vk_renderer->swapchain_images);
    return error;
}
void vulkan_renderer_swapchain_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer == NULL || vk_renderer->swapchain == VK_NULL_HANDLE)
        return;
    vk_renderer->context->vk_loader_device.vkDestroySwapchainKHR(vk_renderer->context->device, vk_renderer->swapchain, NULL);
    vk_renderer->swapchain = VK_NULL_HANDLE;
}

static struct surface_capabilities_result get_surface_capabilities(struct vulkan_loader_instance vk_loader_instance, VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    VkResult result = vk_loader_instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities);
    enum ncbar_vulkan_error error = NCBAR_VULKAN_OK;
    if (result != VK_SUCCESS) {
        error = NCBAR_VULKAN_SURFACE_CAPABILITIES_ERROR;
    }
    return (struct surface_capabilities_result){surface_capabilities, error};
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
            if (surface_formats[i].format == preferred_formats[n] && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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
    VkResult result = vk_loader_instance.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_number, VK_NULL_HANDLE);
    if (result != VK_SUCCESS || present_mode_number == 0) {
        return VK_PRESENT_MODE_MAX_ENUM_KHR;
    }
    VkPresentModeKHR present_modes[present_mode_number];
    memset(present_modes, 0, sizeof(present_modes));
    result = vk_loader_instance.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_number, present_modes);
    if ((result != VK_SUCCESS && result != VK_INCOMPLETE) || present_mode_number == 0) {
        return VK_PRESENT_MODE_MAX_ENUM_KHR;
    }

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
static enum ncbar_vulkan_error vulkan_renderer_swapchain_acquire_images(PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR, VkDevice device, VkSwapchainKHR swapchain,
                                                                        VkImage **swapchain_images_arr_ptr) {
    uint32_t image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    if (result != VK_SUCCESS || image_count == 0) {
        return NCBAR_VULKAN_SWAPCHAIN_IMAGES_FAILED;
    }
    arrsetlen(*swapchain_images_arr_ptr, image_count);
    result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, *swapchain_images_arr_ptr);
    if (result != VK_SUCCESS) {
        return NCBAR_VULKAN_SWAPCHAIN_IMAGES_FAILED;
    }
    return NCBAR_VULKAN_OK;
}