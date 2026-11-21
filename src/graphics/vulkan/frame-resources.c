#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include <stdint.h>
#include <vulkan/vulkan_core.h>

static VkCommandPool create_command_pool(struct vulkan_renderer *vk_renderer);
static VkCommandBuffer create_command_buffer(struct vulkan_renderer *vk_renderer);

enum ncbar_vulkan_error vulkan_renderer_frame_resources_create(struct vulkan_renderer *vk_renderer) {
    vk_renderer->command_pool = create_command_pool(vk_renderer);
    if (vk_renderer->command_pool == VK_NULL_HANDLE) {
        vk_renderer->command_buffer = VK_NULL_HANDLE;
        return NCBAR_VULKAN_COMMAND_POOL_CREATE_FAILED;
    }
    vk_renderer->command_buffer = create_command_buffer(vk_renderer);
    if (vk_renderer->command_buffer == VK_NULL_HANDLE) {
        return NCBAR_VULKAN_COMMAND_BUFFER_ALLOCATE_FAILED;
    }

    return NCBAR_VULKAN_OK;
}

void vulkan_renderer_frame_resources_destroy(struct vulkan_renderer *vk_renderer) {
    vk_renderer->context->vk_loader_device.vkDestroyCommandPool(vk_renderer->context->device, vk_renderer->command_pool, NULL);
}

static VkCommandPool create_command_pool(struct vulkan_renderer *vk_renderer) {
    const uint32_t graphics_queue_family_index = vk_renderer->context->graphics_queue_family;

    const VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family_index,
    };

    VkCommandPool command_pool = VK_NULL_HANDLE;
    if (vk_renderer->context->vk_loader_device.vkCreateCommandPool(vk_renderer->context->device, &create_info, NULL, &command_pool) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return command_pool;
}
static VkCommandBuffer create_command_buffer(struct vulkan_renderer *vk_renderer) {
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_renderer->command_pool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    if (vk_renderer->context->vk_loader_device.vkAllocateCommandBuffers(vk_renderer->context->device, &alloc_info, &command_buffer) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return command_buffer;
}