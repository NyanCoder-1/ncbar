#ifndef NDEBUG
#include "graphics/vulkan/debug_messenger.h"
#include "graphics/vulkan/context.h"

#include <stdio.h>
#include <vulkan/vulkan_core.h>

enum ncbar_vulkan_error vulkan_context_init_debug_messenger(struct vulkan_context *self) {
    if (self == NULL || self->instance == VK_NULL_HANDLE)
        return NCBAR_VULKAN_DEBUG_MESSENGER_INIT_FAILED;
    if (self->vk_loader_instance.vkCreateDebugUtilsMessengerEXT == NULL)
        return NCBAR_VULKAN_DEBUG_MESSENGER_DOESNT_EXIST;
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = NULL;
    VkResult result = self->vk_loader_instance.vkCreateDebugUtilsMessengerEXT(self->instance, &create_info, NULL, &self->debug_messenger);
    return result != VK_SUCCESS ? NCBAR_VULKAN_DEBUG_MESSENGER_INIT_FAILED : NCBAR_VULKAN_OK;
}
void vulkan_context_destroy_debug_messenger(struct vulkan_context *self) {
    if (self == NULL || self->instance == VK_NULL_HANDLE || self->debug_messenger == VK_NULL_HANDLE || self->vk_loader_instance.vkDestroyDebugUtilsMessengerEXT == NULL)
        return;
    self->vk_loader_instance.vkDestroyDebugUtilsMessengerEXT(self->instance, self->debug_messenger, NULL);
    self->debug_messenger = VK_NULL_HANDLE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *userdata) {
    (void)message_type;
    (void)userdata;
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "validation layer: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}
#else
typedef int make_iso_compilers_happy;
#endif