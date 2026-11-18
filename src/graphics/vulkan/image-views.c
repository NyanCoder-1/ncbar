#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <vulkan/vulkan_core.h>

enum ncbar_vulkan_error vulkan_renderer_swapchain_image_views_create(struct vulkan_renderer *vk_renderer) {
    const size_t retire_array_capacity = arrcap(vk_renderer->swapchain_image_view_retire_queue);
    const size_t retire_array_size = arrlenu(vk_renderer->swapchain_image_view_retire_queue);
    const size_t image_views_count = arrlenu(vk_renderer->swapchain_image_views);
    if (retire_array_size + image_views_count > retire_array_capacity) {
        arrsetcap(vk_renderer->swapchain_image_view_retire_queue, retire_array_size + image_views_count);
    }
    for (size_t i = 0; i < image_views_count; i++) {
        arrput(vk_renderer->swapchain_image_view_retire_queue, vk_renderer->swapchain_image_views[i]);
    }
    const size_t images_count = arrlenu(vk_renderer->swapchain_images);
    arrsetlen(vk_renderer->swapchain_image_views, images_count);
    for (size_t i = 0; i < images_count; i++) {
        vk_renderer->swapchain_image_views[i] = VK_NULL_HANDLE;
    }
    for (size_t i = 0; i < images_count; i++) {
        const VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_renderer->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_renderer->swapchain_image_format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        if (vk_renderer->context->vk_loader_device.vkCreateImageView(vk_renderer->context->device, &create_info, NULL, &vk_renderer->swapchain_image_views[i]) != VK_SUCCESS) {
            return NCBAR_VULKAN_SWAPCHAIN_IMAGE_VIEW_CREATE_FAILED;
        }
    }
    return NCBAR_VULKAN_OK;
}
void vulkan_renderer_swapchain_image_views_destroy(struct vulkan_renderer *vk_renderer) {
    const size_t array_size = arrlenu(vk_renderer->swapchain_image_views);
    for (size_t i = 0; i < array_size; i++) {
        vk_renderer->context->vk_loader_device.vkDestroyImageView(vk_renderer->context->device, vk_renderer->swapchain_image_views[i], NULL);
        vk_renderer->swapchain_image_views[i] = VK_NULL_HANDLE;
    }
    arrfree(vk_renderer->swapchain_image_views);
}