#ifndef GRAPHICS_VULKAN_CONTEXT_H
#define GRAPHICS_VULKAN_CONTEXT_H

#include "graphics/vulkan/loader.h"
#include <stdint.h>

struct vulkan_context {
    struct vulkan_loader_global vk_loader_global;
    struct vulkan_loader_instance vk_loader_instance;
    struct vulkan_loader_device vk_loader_device;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    VkDevice device;

    uint32_t graphics_queue_family;
};

enum ncbar_vulkan_error {
    NCBAR_VULKAN_OK,
    NCBAR_VULKAN_DLOPEN_FAILED,
    NCBAR_VULKAN_LOADER_GLOBAL_INIT_FAILED,
    NCBAR_VULKAN_LOADER_INSTANCE_INIT_FAILED,
    NCBAR_VULKAN_LOADER_DEVICE_INIT_FAILED,
    NCBAR_VULKAN_VALIDATION_LAYER_INIT_FAILED,
    NCBAR_VULKAN_INSTANCE_CREATE_FAILED,
    NCBAR_VULKAN_NO_DRIVER,
    NCBAR_VULKAN_EXTENSION_MISSING,
    NCBAR_VULKAN_DEBUG_MESSENGER_DOESNT_EXIST,
    NCBAR_VULKAN_DEBUG_MESSENGER_INIT_FAILED,
    NCBAR_VULKAN_NO_VULKAN_GPU,
    NCBAR_VULKAN_NO_SUITABLE_GPU,
    NCBAR_VULKAN_LOGICAL_DEVICE_CREATE_FAILED,
    NCBAR_VULKAN_SURFACE_INVALID_ARGUMENT,
    NCBAR_VULKAN_SURFACE_CREATE_FAILED,
    NCBAR_VULKAN_SWAPCHAIN_CREATE,
};

enum ncbar_vulkan_error vulkan_context_init(struct vulkan_context *vk_context, struct wl_display *wl_display);
void vulkan_context_destroy(struct vulkan_context *vk_context);

enum ncbar_vulkan_error vulkan_context_init_loader_global(struct vulkan_loader_global *loader);
enum ncbar_vulkan_error vulkan_context_init_loader_instance(VkInstance instance, PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, struct vulkan_loader_instance *loader);
enum ncbar_vulkan_error vulkan_context_init_loader_device(VkDevice device, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, struct vulkan_loader_device *loader);
enum ncbar_vulkan_error vulkan_context_init_instance(struct vulkan_context *self);
enum ncbar_vulkan_error vulkan_context_init_debug_messenger(struct vulkan_context *self);
enum ncbar_vulkan_error vulkan_context_init_physical_device(struct vulkan_context *self, struct wl_display *wl_display);
enum ncbar_vulkan_error vulkan_context_init_device(struct vulkan_context *self);

void vulkan_context_destroy_device(struct vulkan_context *self);
#ifndef NDEBUG
void vulkan_context_destroy_debug_messenger(struct vulkan_context *self);
#endif
void vulkan_context_destroy_instance(struct vulkan_context *self);
void vulkan_context_destroy_loader(struct vulkan_context *self);

#endif