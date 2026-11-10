#ifndef GRAPHICS_VULKAN_DEBUG_MESSENGER_H
#define GRAPHICS_VULKAN_DEBUG_MESSENGER_H

#ifndef NDEBUG
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *userdata);
#endif

#endif