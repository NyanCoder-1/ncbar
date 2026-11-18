#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <vulkan/vulkan_core.h>

enum ncbar_vulkan_error vulkan_renderer_swapchain_framebuffers_create(struct vulkan_renderer *vk_renderer) {
    const size_t retire_array_capacity = arrcap(vk_renderer->swapchain_framebuffer_retire_queue);
    const size_t retire_array_size = arrlenu(vk_renderer->swapchain_framebuffer_retire_queue);
    const size_t framebuffers_count = arrlenu(vk_renderer->swapchain_framebuffers);
    if (retire_array_size + framebuffers_count > retire_array_capacity) {
        arrsetcap(vk_renderer->swapchain_framebuffer_retire_queue, retire_array_size + framebuffers_count);
    }
    for (size_t i = 0; i < framebuffers_count; i++) {
        arrput(vk_renderer->swapchain_framebuffer_retire_queue, vk_renderer->swapchain_framebuffers[i]);
    }
    const size_t images_count = arrlenu(vk_renderer->swapchain_images);
    arrsetlen(vk_renderer->swapchain_framebuffers, images_count);
    for (size_t i = 0; i < images_count; i++) {
        vk_renderer->swapchain_framebuffers[i] = VK_NULL_HANDLE;
    }
    for (size_t i = 0; i < images_count; i++) {
        const VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vk_renderer->render_pass,
            .attachmentCount = 1,
            .pAttachments = (VkImageView[]){vk_renderer->swapchain_image_views[i]},
            .width = vk_renderer->swapchain_extent.width,
            .height = vk_renderer->swapchain_extent.height,
            .layers = 1,
        };
        if (vk_renderer->context->vk_loader_device.vkCreateFramebuffer(vk_renderer->context->device, &create_info, NULL, &vk_renderer->swapchain_framebuffers[i]) != VK_SUCCESS) {
            return NCBAR_VULKAN_SWAPCHAIN_FRAMEBUFFER_CREATE_FAILED;
        }
    }
    return NCBAR_VULKAN_OK;
}
void vulkan_renderer_swapchain_framebuffers_destroy(struct vulkan_renderer *vk_renderer) {
    const size_t array_size = arrlenu(vk_renderer->swapchain_framebuffers);
    for (size_t i = 0; i < array_size; i++) {
        vk_renderer->context->vk_loader_device.vkDestroyFramebuffer(vk_renderer->context->device, vk_renderer->swapchain_framebuffers[i], NULL);
        vk_renderer->swapchain_framebuffers[i] = VK_NULL_HANDLE;
    }
    arrfree(vk_renderer->swapchain_framebuffers);
}