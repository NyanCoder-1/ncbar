#include "graphics/vulkan/context.h"

enum ncbar_vulkan_error vulkan_context_init(struct vulkan_context *vk_context, struct wl_display *wl_display) {
    enum ncbar_vulkan_error error = vulkan_context_init_loader_global(&vk_context->vk_loader_global);
    if (error != NCBAR_VULKAN_OK)
        return error;
    error = vulkan_context_init_instance(vk_context);
    if (error != NCBAR_VULKAN_OK)
        return error;
    error = vulkan_context_init_loader_instance(vk_context->instance, vk_context->vk_loader_global.vkGetInstanceProcAddr, &vk_context->vk_loader_instance);
    if (error != NCBAR_VULKAN_OK)
        return error;
#ifndef NDEBUG
    error = vulkan_context_init_debug_messenger(vk_context);
    if (error != NCBAR_VULKAN_OK)
        return error;
#endif
    error = vulkan_context_init_physical_device(vk_context, wl_display);
    if (error != NCBAR_VULKAN_OK)
        return error;
    error = vulkan_context_init_device(vk_context);
    if (error != NCBAR_VULKAN_OK)
        return error;
    error = vulkan_context_init_loader_device(vk_context->device, vk_context->vk_loader_instance.vkGetDeviceProcAddr, &vk_context->vk_loader_device);
    return error;
}
void vulkan_context_destroy(struct vulkan_context *vk_context) {
    vulkan_context_destroy_device(vk_context);
#ifndef NDEBUG
    vulkan_context_destroy_debug_messenger(vk_context);
#endif
    vulkan_context_destroy_instance(vk_context);
    vulkan_context_destroy_loader(vk_context);
}
