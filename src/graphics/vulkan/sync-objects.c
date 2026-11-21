#include "graphics/vulkan/context.h"
#include "graphics/vulkan/renderer.h"
#include <stb_ds.h>
#include <vulkan/vulkan_core.h>

enum ncbar_vulkan_error vulkan_renderer_semaphore_create(struct vulkan_context *vk_ctx, VkSemaphore *semaphore) {
    const VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    if (vk_ctx->vk_loader_device.vkCreateSemaphore(vk_ctx->device, &semaphore_info, NULL, semaphore) != VK_SUCCESS) {
        return NCBAR_VULKAN_SEMAPHORE_CREATE_FAILED;
    }
    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_renderer_frame_semaphores_create(struct vulkan_renderer *vk_renderer) {
    const size_t retire_array_capacity = arrcap(vk_renderer->vk_semaphore_retire_queue);
    const size_t retire_array_size = arrlenu(vk_renderer->vk_semaphore_retire_queue);
    const size_t render_finished_semaphores_count = arrlenu(vk_renderer->render_finished);
    if (retire_array_size + render_finished_semaphores_count > retire_array_capacity) {
        arrsetcap(vk_renderer->vk_semaphore_retire_queue, retire_array_size + render_finished_semaphores_count);
    }
    for (size_t i = 0; i < render_finished_semaphores_count; i++) {
        arrput(vk_renderer->vk_semaphore_retire_queue, vk_renderer->render_finished[i]);
    }
    const size_t frames_count = arrlenu(vk_renderer->swapchain_images);
    arrsetlen(vk_renderer->render_finished, frames_count);
    for (size_t i = 0; i < frames_count; i++) {
        vk_renderer->render_finished[i] = VK_NULL_HANDLE;
    }
    enum ncbar_vulkan_error error = NCBAR_VULKAN_OK;
    for (size_t i = 0; i < frames_count; i++) {
        error = vulkan_renderer_semaphore_create(vk_renderer->context, &vk_renderer->render_finished[i]);
        if (error != NCBAR_VULKAN_OK) {
            return error;
        }
    }
    return NCBAR_VULKAN_OK;
}
enum ncbar_vulkan_error vulkan_renderer_sync_objects_create(struct vulkan_renderer *vk_renderer) {
    enum ncbar_vulkan_error error = vulkan_renderer_semaphore_create(vk_renderer->context, &vk_renderer->image_available);
    if (error != NCBAR_VULKAN_OK) {
        return error;
    }
    const VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    if (vk_renderer->context->vk_loader_device.vkCreateFence(vk_renderer->context->device, &fence_info, NULL, &vk_renderer->in_flight) != VK_SUCCESS) {
        return NCBAR_VULKAN_FENCE_CREATE_FAILED;
    }
    return NCBAR_VULKAN_OK;
}
void vulkan_renderer_sync_objects_destroy(struct vulkan_renderer *vk_renderer) {
    if (vk_renderer->render_finished != NULL) {
        const size_t array_size = arrlenu(vk_renderer->render_finished);
        for (size_t i = 0; i < array_size; i++) {
            if (vk_renderer->render_finished[i] != VK_NULL_HANDLE) {
                vk_renderer->context->vk_loader_device.vkDestroySemaphore(vk_renderer->context->device, vk_renderer->render_finished[i], NULL);
                vk_renderer->render_finished[i] = VK_NULL_HANDLE;
            }
        }
        arrfree(vk_renderer->render_finished);
    }
    if (vk_renderer->image_available != VK_NULL_HANDLE) {
        vk_renderer->context->vk_loader_device.vkDestroySemaphore(vk_renderer->context->device, vk_renderer->image_available, NULL);
        vk_renderer->image_available = VK_NULL_HANDLE;
    }
    if (vk_renderer->in_flight != VK_NULL_HANDLE) {
        vk_renderer->context->vk_loader_device.vkDestroyFence(vk_renderer->context->device, vk_renderer->in_flight, NULL);
        vk_renderer->in_flight = VK_NULL_HANDLE;
    }
}