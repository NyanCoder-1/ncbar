#include "graphics/vulkan/context.h"
#include "graphics/vulkan/debug_messenger.h"
#include <stdbool.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#ifndef NDEBUG
static bool vulkan_context_check_validation_layer_support(struct vulkan_context *self);
#endif
static enum ncbar_vulkan_error instance_create_result_convert(VkResult result);

#ifndef NDEBUG
static const char *const vk_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
static const uint32_t vk_validation_layers_count = sizeof(vk_validation_layers) / sizeof(vk_validation_layers[0]);
#endif
static const char *const vk_instance_extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#ifndef NDEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};
static const uint32_t vk_instance_extensions_count = sizeof(vk_instance_extensions) / sizeof(vk_instance_extensions[0]);

enum ncbar_vulkan_error vulkan_context_init_instance(struct vulkan_context *self) {
#ifndef NDEBUG
    if (!vulkan_context_check_validation_layer_support(self)) {
        return NCBAR_VULKAN_VALIDATION_LAYER_INIT_FAILED;
    }
#endif
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "ncbar",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

#ifndef NDEBUG
    const VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL,
    };
    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_create_info,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = vk_validation_layers_count,
        .ppEnabledLayerNames = vk_validation_layers,
        .enabledExtensionCount = vk_instance_extensions_count,
        .ppEnabledExtensionNames = vk_instance_extensions,
    };
#else
    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = vk_instance_extensions_count,
        .ppEnabledExtensionNames = vk_instance_extensions,
    };
#endif

    const VkResult result = self->vk_loader_global.vkCreateInstance(&create_info, NULL, &self->instance);
    return instance_create_result_convert(result);
}

void vulkan_context_destroy_instance(struct vulkan_context *self) {
    if (self == NULL || self->instance == VK_NULL_HANDLE || self->vk_loader_instance.vkDestroyInstance == NULL)
        return;
    self->vk_loader_instance.vkDestroyInstance(self->instance, NULL);
    self->instance = VK_NULL_HANDLE;
}

#ifndef NDEBUG
static bool vulkan_context_check_validation_layer_support(struct vulkan_context *self) {
    uint32_t layer_count = 0;
    self->vk_loader_global.vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    const uint32_t layer_array_size = layer_count == 0 ? 1 : layer_count;
    VkLayerProperties available_layers[layer_array_size];
    self->vk_loader_global.vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (uint32_t vk_validation_layer_num = 0; vk_validation_layer_num < vk_validation_layers_count; vk_validation_layer_num++) {
        const char *const layer_name = vk_validation_layers[vk_validation_layer_num];
        uint8_t layer_found = false;

        for (uint32_t available_layers_num = 0; available_layers_num < layer_count; available_layers_num++) {
            const VkLayerProperties layer_properties = available_layers[available_layers_num];
            if (strncmp(layer_name, layer_properties.layerName, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}
#endif
static enum ncbar_vulkan_error instance_create_result_convert(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
        return NCBAR_VULKAN_OK;
    case VK_ERROR_INCOMPATIBLE_DRIVER: // loader found no ICD
        return NCBAR_VULKAN_NO_DRIVER;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return NCBAR_VULKAN_EXTENSION_MISSING;
    default:
        return NCBAR_VULKAN_INSTANCE_CREATE_FAILED;
    }
}